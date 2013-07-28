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

#pragma once

#include <sys/common.h>

inline void SpinLock::aquire(klock_t *l) {
	asm volatile (
		"mov	$1,%%ecx;"
		"1:"
		"	xor		%%eax,%%eax;"
		"	lock	cmpxchg %%ecx,(%0);"
		"	jz		2f;"
		/* improves the performance and lowers the power-consumption of spinlocks */
		"	pause;"
		"	jmp		1b;"
		"2:;"
		/* outputs */
		:
		/* inputs */
		: "D" (l)
		/* eax, ecx and cc will be clobbered; we need memory as well because *l is changed */
		: "eax", "ecx", "cc", "memory"
	);
}

inline void SpinLock::release(klock_t *l) {
	*l = 0;
}