/**
 * $Id: thread.h 900 2011-06-02 20:18:17Z nasmussen $
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#ifndef MMIX_THREAD_H_
#define MMIX_THREAD_H_

#include <esc/common.h>

/* the thread-state which will be saved for context-switching */
typedef struct {
	uintptr_t stackEnd;
	uint64_t rbb;
	uint64_t rww;
	uint64_t rxx;
	uint64_t ryy;
	uint64_t rzz;
} sThreadRegs;

typedef struct {
	/* use as a temporary kernel-stack for cloning */
	tFrameNo tempStack;
	/* when handling a signal, we have to backup these registers */
	struct {
		uint64_t rbb;
		uint64_t rww;
		uint64_t rxx;
		uint64_t ryy;
		uint64_t rzz;
	} sigSave;
} sThreadArchAttr;

#define STACK_REG_COUNT		2

#endif /* MMIX_THREAD_H_ */