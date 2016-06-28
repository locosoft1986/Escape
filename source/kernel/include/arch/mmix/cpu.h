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

#pragma once

#include <common.h>

enum  {
	rA		= 21,	// arithmetic status register
	rB		= 0,	// bootstrap register (trip)
	rC		= 8,	// cycle counter
	rD		= 1,	// dividend register
	rE		= 2,	// epsilon register
	rF		= 22,	// failure location register
	rG		= 19,	// global threshold register
	rH		= 3,	// himult register
	rI		= 12,	// interval register
	rJ		= 4,	// return-jump register
	rK		= 15,	// interrupt mask register
	rL		= 20,	// local threshold register
	rM		= 5,	// multiplex mask register
	rN		= 9,	// serial number
	rO		= 10,	// register stack offset
	rP		= 23,	// prediction register
	rQ		= 16,	// interrupt request register
	rR		= 6,	// remainder register
	rS		= 11,	// register stack pointer
	rT		= 13,	// trap address register
	rU		= 17,	// usage counter
	rV		= 18,	// virtual translation register
	rW		= 24,	// where-interrupted register (trip)
	rX		= 25,	// execution register (trip)
	rY		= 26,	// Y operand (trip)
	rZ		= 27,	// Z operand (trip)
	rBB		= 7,	// bootstrap register (trap)
	rTT		= 14,	// dynamic trap address register
	rWW		= 28,	// where interrupted register (trap)
	rXX		= 29,	// execution register (trap)
	rYY		= 30,	// Y operand (trap)
	rZZ		= 31,	// Z operand (trap)
	rSS		= 32,	// kernel-stack location (GIMMIX only)
};

class CPU : public CPUBase {
	friend class CPUBase;

	CPU() = delete;

public:
	/**
	 * @param rno the special-number
	 * @return the name of the given special-register
	 */
	static const char *getSpecialName(int rno) {
		if(rno >= (int)ARRAY_SIZE(specialRegs))
			return "??";
		return specialRegs[rno];
	}

	/**
	 * Sets the given CPU-speed (no, the hardware is not changed ;))
	 *
	 * @param hz the CPU-speed in Hz
	 */
	static void setSpeed(uint64_t hz) {
		cpuHz = hz;
	}

	/**
	 * Sets rbb, rww, rxx, ryy and rzz to the current values of rBB, rWW, rXX, rYY and rZZ,
	 * respectively.
	 *
	 * @param rbb the pointer to rBB
	 * @param rww the pointer to rWW
	 * @param rxx the pointer to rXX
	 * @param ryy the pointer to rYY
	 * @param rzz the pointer to rZZ
	 */
	static void getKSpecials(uint64_t *rbb,uint64_t *rww,uint64_t *rxx,uint64_t *ryy, uint64_t *rzz) {
		asm volatile ("GET	%0, rBB" : "=r"(*rbb));
		asm volatile ("GET	%0, rWW" : "=r"(*rww));
		asm volatile ("GET	%0, rXX" : "=r"(*rxx));
		asm volatile ("GET	%0, rYY" : "=r"(*ryy));
		asm volatile ("GET	%0, rZZ" : "=r"(*rzz));
	}

	/**
	 * Sets rBB, rWW, rXX, rYY and rZZ to the current values of rbb, rww, rxx, ryy and rzz,
	 * respectively.
	 *
	 * @param rbb the new value of rBB
	 * @param rww the new value of rWW
	 * @param rxx the new value of rXX
	 * @param ryy the new value of rYY
	 * @param rzz the new value of rZZ
	 */
	static void setKSpecials(uint64_t rbb,uint64_t rww,uint64_t rxx,uint64_t ryy,uint64_t rzz) {
		asm volatile ("PUT	rBB, %0" : : "r"(rbb));
		asm volatile ("PUT	rWW, %0" : : "r"(rww));
		asm volatile ("PUT	rXX, %0" : : "r"(rxx));
		asm volatile ("PUT	rYY, %0" : : "r"(ryy));
		asm volatile ("PUT	rZZ, %0" : : "r"(rzz));
	}

	/**
	 * @return the fault-location for protection-faults
	 */
	static uintptr_t getFaultLoc() {
		uintptr_t res;
		asm volatile ("GET %0, rYY" : "=r"(res));
		return res;
	}

	/**
	 * Performs a SYNCD and SYNCID for the given region to ensure that the IC agrees with the DC
	 *
	 * @param start the start-address
	 * @param end the end-address
	 */
	static void syncid(uintptr_t start,uintptr_t end) asm("cpu_syncid");

	/**
	 * Retrieves the value of the global-register with given number
	 *
	 * @param rno the global-number
	 * @return the value
	 */
	static uint64_t getGlobal(int rno) asm("cpu_getGlobal");

	/**
	 * Retrieves the value of the special-register with given number
	 *
	 * @param rno the special-number
	 * @return the value
	 */
	static uint64_t getSpecial(int rno) asm("cpu_getSpecial");

private:
	static const char *specialRegs[33];
	static uint64_t cpuHz;
};

inline void CPUBase::halt() {
	while(1)
		;
}

inline uint64_t CPUBase::rdtsc() {
	uint64_t res;
	asm volatile ("GET %0, rC" : "=r"(res));
	return res;
}

inline uint64_t CPUBase::getSpeed() {
	return CPU::cpuHz;
}
