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
#include <sys/arch/i586/mem/paging.h>
#include <sys/arch/i586/gdt.h>
#include <sys/arch/i586/idt.h>
#include <sys/arch/i586/fpu.h>
#include <sys/arch/i586/apic.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/task/elf.h>
#include <sys/task/smp.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/paging.h>
#include <sys/cpu.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <sys/boot.h>
#include <sys/util.h>
#include <assert.h>

/* make gcc happy */
EXTERN_C void bspstart(sBootInfo *mbp);
EXTERN_C uintptr_t smpstart(void);
EXTERN_C void apstart(void);
static void idlestart(void);
extern klock_t aplock;

static uint8_t initloader[] = {
#if DEBUGGING
#	include "../../../../build/i586-debug/user_initloader.dump"
#else
#	include "../../../../build/i586-release/user_initloader.dump"
#endif
};

void bspstart(sBootInfo *mbp) {
	/* init the kernel */
	boot_start(mbp);
}

uintptr_t smpstart(void) {
	Thread *t;
	ELF::StartupInfo info;
	size_t i,total = SMP::getCPUCount();
	/* the running thread has been stored on a different stack last time */
	Thread::setRunning(Thread::getById(0));

	/* start an idle-thread for each cpu */
	for(i = 0; i < total; i++)
		Proc::startThread((uintptr_t)&idlestart,T_IDLE,NULL);

	/* start all APs */
	SMP::start();

	/* load initloader */
	if(ELF::loadFromMem(initloader,sizeof(initloader),&info) < 0)
		util_panic("Unable to load initloader");
	/* give the process some stack pages */
	t = Thread::getRunning();
	if(!t->reserveFrames(INITIAL_STACK_PAGES))
		util_panic("Not enough mem for initloader-stack");
	t->addInitialStack();
	t->discardFrames();
	return info.progEntry;
}

void apstart(void) {
	Proc *p = Proc::getByPid(0);
	/* store the running thread for our temp-stack again, because we might need it in gdt_init_ap
	 * for example */
	Thread::setRunning(Thread::getById(0));
	/* at first, activate paging and setup the GDT, so that we don't need the "GDT-trick" anymore */
	paging_activate(p->getPageDir()->own);
	gdt_init_ap();
	/* setup IDT for this cpu and enable its local APIC */
	idt_init();
	apic_enable();
	/* init FPU and detect our CPU */
	fpu_preinit();
	CPU::detect();
	/* notify the BSP that we're running */
	SMP::apIsRunning();
	/* choose a thread to run */
	Thread::initialSwitch();
}

static void idlestart(void) {
	if(!SMP::isBSP()) {
		/* unlock the temporary kernel-stack, so that other CPUs can use it */
		spinlock_release(&aplock);
	}
	thread_idle();
}