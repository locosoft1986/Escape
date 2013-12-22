/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sys/common.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/task/sched.h>
#include <sys/task/timer.h>
#include <sys/task/smp.h>
#include <sys/arch/i586/gdt.h>
#include <sys/arch/i586/fpu.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/pagedir.h>
#include <sys/log.h>
#include <sys/spinlock.h>
#include <sys/config.h>
#include <sys/cpu.h>
#include <assert.h>
#include <errno.h>

static klock_t switchLock;

int ThreadBase::initArch(Thread *t) {
	t->kernelStack = t->getProc()->getPageDir()->createKernelStack();
	t->fpuState = NULL;
	return 0;
}

void ThreadBase::addInitialStack() {
	assert(tid == INIT_TID);
	int res = proc->getVM()->map(0,INITIAL_STACK_PAGES * PAGE_SIZE,0,PROT_READ | PROT_WRITE,
			MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN,NULL,0,stackRegions + 0);
	assert(res == 0);
}

int ThreadBase::createArch(const Thread *src,Thread *dst,bool cloneProc) {
	if(cloneProc) {
		/* map the kernel-stack at the same address */
		if(dst->getProc()->getPageDir()->map(src->kernelStack,NULL,1,
				PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR) < 0)
			return -ENOMEM;
		dst->kernelStack = src->kernelStack;
	}
	else {
		/* map the kernel-stack at a free address */
		dst->kernelStack = dst->getProc()->getPageDir()->createKernelStack();
		if(dst->kernelStack == 0)
			return -ENOMEM;

		/* add a new stack-region */
		int res = dst->getProc()->getVM()->map(0,INITIAL_STACK_PAGES * PAGE_SIZE,0,PROT_READ | PROT_WRITE,
				MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN,NULL,0,dst->stackRegions + 0);
		if(res < 0) {
			dst->getProc()->getPageDir()->unmap(dst->kernelStack,1,true);
			return res;
		}
	}
	FPU::cloneState(&(dst->fpuState),src->fpuState);
	return 0;
}

void ThreadBase::freeArch(Thread *t) {
	if(t->stackRegions[0] != NULL) {
		t->getProc()->getVM()->remove(t->stackRegions[0]);
		t->stackRegions[0] = NULL;
	}
	t->getProc()->getPageDir()->removeKernelStack(t->kernelStack);
	FPU::freeState(&t->fpuState);
}

int ThreadBase::finishClone(Thread *t,Thread *nt) {
	/* ensure that we won't get interrupted */
	klock_t lock = 0;
	SpinLock::acquire(&lock);
	/* we clone just the current thread. all other threads are ignored */
	/* map stack temporary (copy later) */
	frameno_t frame = nt->getProc()->getPageDir()->getFrameNo(nt->kernelStack);
	ulong *dst = (ulong*)PageDir::mapToTemp(&frame,1);

	if(Thread::save(&nt->saveArea)) {
		/* child */
		return 1;
	}

	/* now copy the stack */
	/* copy manually to prevent a function-call (otherwise we would change the stack) */
	ulong *src = (ulong*)t->kernelStack;
	for(size_t i = 0; i < (PAGE_SIZE / sizeof(ulong)) - 1; i++)
		*dst++ = *src++;
	/* store thread at the top */
	*dst = (ulong)nt;

	PageDir::unmapFromTemp(1);
	SpinLock::release(&lock);
	/* parent */
	return 0;
}

void ThreadBase::finishThreadStart(A_UNUSED Thread *t,Thread *nt,const void *arg,uintptr_t entryPoint) {
	/* setup kernel-stack */
	frameno_t frame = nt->getProc()->getPageDir()->getFrameNo(nt->kernelStack);
	ulong *dst = (ulong*)PageDir::mapToTemp(&frame,1);
	ulong sp = nt->kernelStack + PAGE_SIZE - sizeof(ulong) * 6;
	dst += (PAGE_SIZE / sizeof(ulong)) - 1;
	*dst = (ulong)nt;
	*--dst = nt->getProc()->getEntryPoint();
	*--dst = entryPoint;
	*--dst = (ulong)arg;
	*--dst = (ulong)&Thread::startup;
	*--dst = sp;
	PageDir::unmapFromTemp(1);

	/* prepare registers for the first Thread::resume() */
	nt->saveArea.ebp = sp;
	nt->saveArea.esp = sp;
	nt->saveArea.ebx = 0;
	nt->saveArea.edi = 0;
	nt->saveArea.esi = 0;
	nt->saveArea.eflags = 0;
}

bool ThreadBase::beginTerm() {
	SpinLock::acquire(&switchLock);
	/* at first the thread can't run to do that. if its not running, its important that no resources
	 * or heap-allocations are hold. otherwise we would produce a deadlock or memory-leak */
	bool res = state != Thread::RUNNING && !hasResources();
	/* ensure that the thread won't be chosen again */
	if(res)
		Sched::removeThread(static_cast<Thread*>(this));
	SpinLock::release(&switchLock);
	return res;
}

void Thread::initialSwitch() {
	SpinLock::acquire(&switchLock);
	cpuid_t cpu = GDT::getCPUId();
	Thread *cur = Sched::perform(NULL,cpu);
	cur->stats.schedCount++;
	if(PhysMem::shouldSetRegTimestamp())
		VirtMem::setTimestamp(cur,Timer::getTimestamp());
	GDT::prepareRun(cpu,true,cur);
	cur->setCPU(cpu);
	FPU::lockFPU();
	cur->stats.cycleStart = CPU::rdtsc();
	Thread::resume(cur->getProc()->getPageDir()->getPhysAddr(),&cur->saveArea,&switchLock,true);
}

void ThreadBase::doSwitch() {
	Thread *old = Thread::getRunning();
	/* lock this, because Sched::perform() may make us ready and we can't be chosen by another CPU
	 * until we've really switched the thread (kernelstack, ...) */
	SpinLock::acquire(&switchLock);

	/* update runtime-stats */
	uint64_t cycles = CPU::rdtsc();
	uint64_t runtime = cycles - old->stats.cycleStart;
	old->stats.runtime += runtime;
	old->stats.curCycleCount += runtime;
	cpuid_t cpu = old->getCPU();

	/* choose a new thread to run */
	Thread *n = Sched::perform(old,cpu);
	n->stats.schedCount++;

	/* switch thread */
	if(EXPECT_TRUE(n->getTid() != old->getTid())) {
		if(EXPECT_FALSE(PhysMem::shouldSetRegTimestamp()))
			VirtMem::setTimestamp(n,cycles);
		GDT::prepareRun(cpu,n->getProc() != old->getProc(),n);
		if(cpu != n->getCPU())
			n->getStats().migrations++;
		n->setCPU(cpu);

		/* some stats for SMP */
		SMP::schedule(cpu,n,cycles);

		/* lock the FPU so that we can save the FPU-state for the previous process as soon
		 * as this one wants to use the FPU */
		FPU::lockFPU();
		if(!Thread::save(&old->saveArea)) {
			/* old thread */
			n->stats.cycleStart = CPU::rdtsc();
			Thread::resume(n->getProc()->getPageDir()->getPhysAddr(),&n->saveArea,&switchLock,
			               n->getProc() != old->getProc());
		}
	}
	else {
		SMP::schedule(cpu,n,cycles);
		n->stats.cycleStart = CPU::rdtsc();
		SpinLock::release(&switchLock);
	}
}

void ThreadBase::printState(OStream &os,const ThreadRegs *st) const {
	os.writef("State @ 0x%08Px:\n",st);
	os.writef("\tesp = %#08x\n",st->esp);
	os.writef("\tedi = %#08x\n",st->edi);
	os.writef("\tesi = %#08x\n",st->esi);
	os.writef("\tebp = %#08x\n",st->ebp);
	os.writef("\teflags = %#08x\n",st->eflags);
}
