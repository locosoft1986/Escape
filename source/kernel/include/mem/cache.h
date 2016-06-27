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
#include <mem/layout.h>
#include <spinlock.h>

class OStream;

class Cache {
	Cache() = delete;

	struct Entry {
		const size_t objSize;
		size_t totalObjs;
		size_t freeObjs;
		void *freeList;
	};

public:
	/**
	 * Allocates <size> bytes from the cache
	 *
	 * @param size the size
	 * @return the pointer to the allocated area or NULL
	 */
	static void *alloc(size_t size) asm("cache_alloc");

	/**
	 * Allocates <num> objects with <size> bytes from the cache and zero's the memory.
	 *
	 * @param num the number of objects
	 * @param size the size of an object
	 * @return the pointer to the allocated area or NULL
	 */
	static void *calloc(size_t num,size_t size) asm("cache_calloc");

	/**
	 * Reallocates the area at <p> to be <size> bytes large. This may involve moving it to a different
	 * location.
	 *
	 * @param p the area
	 * @param size the new size of the area
	 * @return the potentially new location of the area or NULL
	 */
	static void *realloc(void *p,size_t size) asm("cache_realloc");

	/**
	 * Frees the given area
	 *
	 * @param p the area
	 */
	static void free(void *p) asm("cache_free");

	/**
	 * Note: on MMIX, this does only check whether it's in kernel space, but not in static data
	 *
	 * @param p the pointer
	 * @return whether the given pointer belongs to the cache/kheap
	 */
	static bool contains(const void *p) {
		uintptr_t addr = reinterpret_cast<uintptr_t>(p);
		return addr >= KHEAP_START && addr < KHEAP_START + KHEAP_SIZE;
	}

	/**
	 * @return the occupied memory
	 */
	static size_t getOccMem();

	/**
	 * @return the used memory
	 */
	static size_t getUsedMem();

	/**
	 * Prints the cache
	 *
	 * @param os the output-stream
	 */
	static void print(OStream &os);

	#if DEBUGGING
	/**
	 * Enables/disables "allocate and free" prints
	 *
	 * @param enabled the new value
	 */
	static void dbg_setAaFEnabled(bool enabled) {
		aafEnabled = enabled;
	}
	#endif

private:
	static size_t totalObjSize(size_t sz);
	static void printBar(OStream &os,size_t mem,size_t maxMem,size_t total,size_t free);
	static void *get(Entry *c,size_t i);

#if DEBUGGING
	static bool aafEnabled;
#endif
	static SpinLock lock;
	static Entry caches[];
};
