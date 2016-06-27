/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <mem/cache.h>
#include <mem/pagedir.h>
#include <task/filedesc.h>
#include <task/proc.h>
#include <task/sched.h>
#include <task/sems.h>
#include <task/signals.h>
#include <task/thread.h>
#include <task/timer.h>
#include <vfs/vfs.h>
#include <vfs/irq.h>
#include <common.h>
#include <errno.h>
#include <string.h>
#include <syscalls.h>
#include <util.h>

int Syscalls::gettid(Thread *t,IntrptStackFrame *stack) {
	SYSC_RET1(stack,t->getTid());
}

int Syscalls::startthread(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	uintptr_t entryPoint = SYSC_ARG1(stack);
	void *arg = (void*)SYSC_ARG2(stack);
	if(EXPECT_FALSE(!PageDir::isInUserSpace(entryPoint,1)))
		SYSC_ERROR(stack, -EINVAL);

	int res = Proc::startThread(entryPoint,0,arg);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::exit(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	int exitCode = (int)SYSC_ARG1(stack);
	Proc::terminateThread(exitCode);
	A_UNREACHED;
}

int Syscalls::getcycles(Thread *t,IntrptStackFrame *stack) {
	uint64_t *res = (uint64_t*)SYSC_ARG1(stack);
	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)res,sizeof(uint64_t))))
		SYSC_ERROR(stack,-EFAULT);
	*res = t->getStats().curCycleCount;
	SYSC_RET1(stack,0);
}

int Syscalls::alarm(Thread *t,IntrptStackFrame *stack) {
	time_t usecs = SYSC_ARG1(stack);
	int res;
	/* ensure that we're not already in the list */
	Timer::removeThread(t->getTid());
	/* TODO support microseconds */
	if(EXPECT_FALSE((res = Timer::sleepFor(t->getTid(),usecs / 1000,false)) < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int Syscalls::sleep(Thread *t,IntrptStackFrame *stack) {
	time_t usecs = SYSC_ARG1(stack);
	int res;
	/* TODO support microseconds */
	if(EXPECT_FALSE((res = Timer::sleepFor(t->getTid(),usecs / 1000,true)) < 0))
		SYSC_ERROR(stack,res);
	Thread::switchAway();
	/* ensure that we're no longer in the timer-list. this may for example happen if we get a signal
	 * and the sleep-time was not over yet. */
	Timer::removeThread(t->getTid());
	if(EXPECT_FALSE(t->hasSignal()))
		SYSC_ERROR(stack,-EINTR);
	SYSC_RET1(stack,0);
}

int Syscalls::yield(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	Thread::switchAway();
	SYSC_RET1(stack,0);
}

int Syscalls::join(Thread *t,IntrptStackFrame *stack) {
	tid_t tid = (tid_t)SYSC_ARG1(stack);
	if(tid != 0) {
		const Thread *tt = Thread::getById(tid);
		/* just threads from the own process */
		if(EXPECT_FALSE(tt == NULL || tt->getTid() == t->getTid() ||
				tt->getProc()->getPid() != t->getProc()->getPid()))
			SYSC_ERROR(stack,-EINVAL);
	}

	int res = Proc::join(tid);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int Syscalls::semcrtirq(Thread *t,IntrptStackFrame *stack) {
	char kname[32];
	int fd = (int)SYSC_ARG1(stack);
	char *name = (char*)SYSC_ARG2(stack);
	uint64_t *msiaddr = (uint64_t*)SYSC_ARG3(stack);
	uint32_t *msival = (uint32_t*)SYSC_ARG4(stack);
	uint64_t kmsiaddr;
	uint32_t kmsival;
	int irq,res;
	pid_t pid = t->getProc()->getPid();

	if(EXPECT_FALSE(msiaddr && !PageDir::isInUserSpace((uintptr_t)msiaddr,sizeof(*msiaddr))))
		SYSC_ERROR(stack,-EINVAL);
	if(EXPECT_FALSE(msiaddr && !PageDir::isInUserSpace((uintptr_t)msival,sizeof(*msival))))
		SYSC_ERROR(stack,-EINVAL);

	/* get IRQ file */
	OpenFile *irqFile = FileDesc::request(t->getProc(),fd);
	if(EXPECT_FALSE(irqFile == NULL))
		SYSC_ERROR(stack,-EBADF);

	/* it has to be an IRQ */
	if(irqFile->getDev() != VFS_DEV_NO || !S_ISIRQ(irqFile->getNode()->getMode())) {
		res = -EINVAL;
		goto errFile;
	}
	/* and we need exec permissions to get notified about irqs */
	res = VFS::hasAccess(pid,irqFile->getNode(),VFS_EXEC);
	if(res < 0)
		goto errFile;

	/* get irq number */
	irq = static_cast<VFSIRQ*>(irqFile->getNode())->getIRQ();
	FileDesc::release(irqFile);

	/* create semaphore and attach it */
	strnzcpy(kname,name,sizeof(kname));
	res = Sems::create(t->getProc(),0,irq,kname,msiaddr ? &kmsiaddr : NULL,msiaddr ? &kmsival : NULL);
	if(res < 0)
		SYSC_ERROR(stack,res);

	if(msiaddr) {
		*msiaddr = kmsiaddr;
		*msival = kmsival;
	}
	SYSC_RET1(stack,res);

errFile:
	FileDesc::release(irqFile);
	SYSC_ERROR(stack,res);
}

int Syscalls::semcrt(Thread *t,IntrptStackFrame *stack) {
	uint value = (uint)SYSC_ARG1(stack);
	int res = Sems::create(t->getProc(),value);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::semop(Thread *t,IntrptStackFrame *stack) {
	int sem = (int)SYSC_ARG1(stack);
	int amount = (int)SYSC_ARG2(stack);
	int res = Sems::op(t->getProc(),sem,amount);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int Syscalls::semdestr(Thread *t,IntrptStackFrame *stack) {
	int sem = (int)SYSC_ARG1(stack);
	Sems::destroy(t->getProc(),sem);
	SYSC_RET1(stack,0);
}
