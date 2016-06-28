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

#include <mem/kheap.h>
#include <mem/pagedir.h>
#include <mem/physmem.h>
#include <common.h>

uintptr_t KHeap::allocAreas() {
	frameno_t frame = PhysMem::allocate(PhysMem::CRIT);
	if(frame == PhysMem::INVALID_FRAME)
		return 0;
	pages++;
	return DIR_MAP_AREA | (frame * PAGE_SIZE);
}

uintptr_t KHeap::allocSpace(size_t count) {
	/* if its just one page, take a frame from the pmem-stack */
	if(count == 1)
		return allocAreas();
	/* otherwise we have to use contiguous physical memory */
	ssize_t res = PhysMem::allocateContiguous(count,1);
	if(res < 0)
		return 0;
	pages += count;
	return DIR_MAP_AREA | (res * PAGE_SIZE);
}
