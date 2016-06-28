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

#include <mem/dynarray.h>
#include <mem/pagedir.h>
#include <mem/physmem.h>
#include <common.h>
#include <string.h>
#include <util.h>

bool DynArray::extend() {
	/* we use the begin for the current size here */
	if(regions == NULL)
		areaBegin = 0;

	/* region full? */
	if(areaBegin + PAGE_SIZE > areaSize)
		return false;

	/* allocate new region */
	Region *reg = freeList;
	if(reg == NULL)
		return false;

	/* allocate a frame */
	frameno_t frame = PhysMem::allocate(PhysMem::CRIT);
	if(frame == PhysMem::INVALID_FRAME)
		return false;

	/* remove from freelist */
	freeList = freeList->next;
	reg->next = NULL;
	/* append to regions */
	if(regions == NULL)
		regions = reg;
	else {
		Region *r = regions;
		while(r->next != NULL)
			r = r->next;
		r->next = reg;
	}

	reg->addr = DIR_MAP_AREA | (frame * PAGE_SIZE);
	reg->size = PAGE_SIZE;
	totalPages++;
	/* clear it and increase total size and number of objects */
	memclear((void*)reg->addr,PAGE_SIZE);
	areaBegin += PAGE_SIZE;
	objCount += PAGE_SIZE / objSize;
	return true;
}

DynArray::~DynArray() {
	Region *reg = regions;
	while(reg != NULL) {
		Region *next = reg->next;
		PhysMem::free((reg->addr & ~DIR_MAP_AREA) / PAGE_SIZE,PhysMem::CRIT);
		totalPages--;
		/* put region on freelist */
		reg->next = freeList;
		freeList = reg;
		reg = next;
	}
	regions = NULL;
}
