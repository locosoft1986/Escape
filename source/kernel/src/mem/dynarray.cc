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
#include <common.h>
#include <lockguard.h>

DynArray::Region DynArray::regionstore[REG_COUNT];
DynArray::Region *DynArray::freeList;
size_t DynArray::totalPages = 0;

void DynArray::init() {
	regionstore[0].next = NULL;
	freeList = regionstore;
	for(size_t i = 1; i < REG_COUNT; i++) {
		regionstore[i].next = freeList;
		freeList = regionstore + i;
	}
}

void *DynArray::getObj(size_t index) const {
	LockGuard<SpinLock> g(&lock);
	Region *reg = regions;
	/* note that we're using the index here to prevent that an object reaches out of a region */
	while(reg != NULL) {
		size_t objsInReg = reg->size / objSize;
		if(index < objsInReg)
			return (void*)(reg->addr + index * objSize);
		index -= objsInReg;
		reg = reg->next;
	}
	return NULL;
}

ssize_t DynArray::getIndex(const void *obj) const {
	LockGuard<SpinLock> g(&lock);
	size_t index = 0;
	Region *reg = regions;
	while(reg != NULL) {
		if((uintptr_t)obj >= reg->addr && (uintptr_t)obj < reg->addr + reg->size)
			return index + ((uintptr_t)obj - reg->addr) / objSize;
		index += reg->size / objSize;
		reg = reg->next;
	}
	return -1;
}
