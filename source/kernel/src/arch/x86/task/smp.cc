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

#include <arch/x86/acpi.h>
#include <arch/x86/gdt.h>
#include <arch/x86/ioapic.h>
#include <arch/x86/lapic.h>
#include <arch/x86/mpconfig.h>
#include <mem/cache.h>
#include <mem/pagedir.h>
#include <task/smp.h>
#include <task/timer.h>
#include <common.h>
#include <config.h>
#include <cpu.h>
#include <log.h>
#include <spinlock.h>
#include <string.h>
#include <util.h>
#include <video.h>

static const uintptr_t TRAMPOLINE_ADDR		= 0x7000;

EXTERN_C void apEntry();

cpuid_t *SMP::log2Phys;

static SpinLock smpLock;
static volatile size_t seenAPs = 0;

extern volatile uint waiting;
extern volatile uint waitlock;
extern volatile uint halting;
extern volatile uint flushed;

bool SMPBase::initArch() {
	enabled = Config::get(Config::SMP);
	if(LAPIC::isAvailable()) {
		/* if ACPI is not available, try MPConf */
		if(!ACPI::isEnabled() && MPConfig::find())
			MPConfig::parse();
	}

	/* if we have not found CPUs by ACPI or MPConf, assume we have only the BSP */
	if(getCPUCount() == 0) {
		addCPU(true,0,true);
		setId(0,0);
	}

	/* from now on, we'll use the logical-id as far as possible; but remember the physical
	 * one for IPIs, e.g. */
	cpuid_t id = LAPIC::getId();
	SMP::log2Phys = (cpuid_t*)Cache::alloc(getCPUCount() * sizeof(cpuid_t));
	SMP::log2Phys[0] = id;
	return enabled;
}

void SMPBase::pauseOthers() {
retry:
	if(smpLock.tryDown()) {
		cpuid_t cur = getCurId();
		/* wait until all CPUs left the last wait. otherwise, if we're too fast so that they haven't
		 * done that yet, they might get stuck in the loop that waits for waitlock to become 0
		 * (because we set it to 1 below) */
		while(waiting > 0)
			::CPU::pause();
		/* now a new pause can begin */
		waitlock = 1;
		size_t count = 0;
		for(auto cpu = begin(); cpu != end(); ++cpu) {
			if(cpu->id != cur && cpu->ready) {
				sendIPI(cpu->id,IPI_WAIT);
				count++;
			}
		}
		/* wait until all CPUs are paused */
		while(waiting < count)
			::CPU::pause();
		smpLock.up();
	}
	else {
		Atomic::fetch_and_add(&waiting,+1);
		while(waitlock)
			::CPU::pause();
		Atomic::fetch_and_add(&waiting,-1);
		goto retry;
	}
}

void SMPBase::resumeOthers() {
	waitlock = 0;
}

void SMPBase::haltOthers() {
	if(smpLock.tryDown()) {
		cpuid_t cur = getCurId();
		halting = 0;
		size_t count = 0;
		for(auto cpu = begin(); cpu != end(); ++cpu) {
			if(cpu->id != cur && cpu->ready) {
				/* prevent that we want to halt/pause this one again */
				cpu->ready = false;
				sendIPI(cpu->id,IPI_HALT);
				count++;
			}
		}
		/* wait until all CPUs are halted */
		while(halting < count)
			::CPU::pause();
		smpLock.up();
	}
	else {
		Atomic::fetch_and_add(&halting,+1);
		while(1)
			::CPU::halt();
	}
}

void SMPBase::ensureTLBFlushed() {
	if(smpLock.tryDown()) {
		cpuid_t cur = getCurId();
		flushed = 0;
		size_t count = 0;
		for(auto cpu = begin(); cpu != end(); ++cpu) {
			if(cpu->id != cur && cpu->ready) {
				sendIPI(cpu->id,IPI_FLUSH_TLB_ACK);
				count++;
			}
		}
		/* wait until all CPUs have flushed their TLB */
		while(halting == 0 && flushed < count)
			::CPU::pause();
		smpLock.up();
	}
	else {
		::CPU::setCR3(::CPU::getCR3());
		Atomic::fetch_and_add(&flushed,+1);
	}
}

void SMP::apIsRunning() {
	smpLock.down();
	cpuid_t phys = LAPIC::getId();
	cpuid_t log = GDT::getCPUId();
	log2Phys[log] = phys;
	setId(phys,log);
	setReady(log);
	seenAPs++;
	smpLock.up();
}

void SMPBase::start() {
	if(isEnabled()) {
		/* TODO thats not completely correct, according to the MP specification */
		/* we have to check if apic is an 82489DX */

		LAPIC::sendInitIPI();
		Timer::wait(10000);

		LAPIC::sendStartupIPI(TRAMPOLINE_ADDR);
		Timer::wait(200);
		LAPIC::sendStartupIPI(TRAMPOLINE_ADDR);
		Timer::wait(200);

		/* wait until all APs are running */
		size_t total = getCPUCount() - 1;
		uint64_t start = ::CPU::rdtsc();
		uint64_t end = start + Timer::timeToCycles(2000000);
		while(::CPU::rdtsc() < end && seenAPs != total)
			::CPU::pause();
		if(seenAPs != total) {
			Log::get().writef("Found %zu CPUs in 2s, expected %zu. Disabling SMP\n",seenAPs,total);
			disable();
		}
	}

	setReady(0);
}
