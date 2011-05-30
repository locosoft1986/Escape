/**
 * $Id$
 */

#ifndef ECO32_PAGING_H_
#define ECO32_PAGING_H_

#include <esc/common.h>

/**
 * Virtual memory layout:
 * 0x00000000: +-----------------------------------+   -----
 *             |                text               |     |
 *             +-----------------------------------+     |
 *             |               rodata              |     |
 *     ---     +-----------------------------------+     |
 *             |                data               |     |
 *      |      |                                   |
 *      v      |                ...                |     u
 *     ---     +-----------------------------------+     s
 *      ^      |                                   |     e
 *      |      |        user-stack thread n        |     r
 *             |                                   |     a
 *     ---     +-----------------------------------+     r
 *             |                ...                |     e
 *     ---     +-----------------------------------+     a
 *      ^      |                                   |
 *      |      |        user-stack thread 0        |     |
 *             |                                   |     |
 * 0x60000000: +-----------------------------------+     |
 *             |  dynlinker (always first, if ex)  |     |
 *             |           shared memory           |     |
 *             |         shared libraries          |     |
 *             |          shared lib data          |     |
 *             |       thread local storage        |     |
 *             |        memory mapped stuff        |     |
 *             |        (in arbitrary order)       |     |
 * 0x80000000: +-----------------------------------+   -----    -----
 *             |         mapped page-tables        |     |        |
 * 0x80400000: +-----------------------------------+     |     not shared pts
 *             |      temp mapped page-tables      |     |        |
 * 0x80800000: +-----------------------------------+            -----
 *             |                                   |     k
 *      |      |            kernel-heap            |     e
 *      v      |                                   |     r
 * 0x80C00000: +-----------------------------------+     r      -----
 *             |       VFS global file table       |     e        |
 * 0x81000000: +-----------------------------------+     a
 *             |             VFS nodes             |          dynamically extending regions
 * 0x81400000: +-----------------------------------+     |
 *             |             sll nodes             |     |        |
 * 0x83400000: +-----------------------------------+     |      -----
 *             |           temp map area           |     |
 * 0x84400000: +-----------------------------------+     |      -----
 *             |           kernel stack            |     |     not shared
 * 0x84401000: +-----------------------------------+     |      -----
 *             |                ...                |     |
 * 0xC0000000: +-----------------------------------+   -----
 *             |         kernel code+data          |     |
 *             +-----------------------------------+
 *      |      |             mm-stack              |    directly mapped space
 *      v      +-----------------------------------+
 *             |                ...                |     |
 * 0xFFFFFFFF: +-----------------------------------+   -----
 */

/* the virtual address of the kernel-area */
#define KERNEL_AREA_V_ADDR		((uintptr_t)0x80000000)
#define DIR_MAPPED_SPACE		((uintptr_t)0xC0000000)

/* the number of entries in a page-directory or page-table */
#define PT_ENTRY_COUNT			(PAGE_SIZE / 4)

/* the start of the mapped page-tables area */
#define MAPPED_PTS_START		KERNEL_AREA_V_ADDR
/* the start of the temporary mapped page-tables area */
#define TMPMAP_PTS_START		(MAPPED_PTS_START + (PT_ENTRY_COUNT * PAGE_SIZE))

/* page-directories in virtual memory */
#define PAGE_DIR_AREA			(MAPPED_PTS_START + PAGE_SIZE * 512)
/* needed for building a new page-dir */
#define PAGE_DIR_TMP_AREA		(TMPMAP_PTS_START + PAGE_SIZE * 513)

/* the start of the kernel-heap */
#define KERNEL_HEAP_START		(TMPMAP_PTS_START + (PT_ENTRY_COUNT * PAGE_SIZE))
/* the size of the kernel-heap (4 MiB) */
#define KERNEL_HEAP_SIZE		(PT_ENTRY_COUNT * PAGE_SIZE)

/* area for global-file-table */
#define GFT_AREA				(KERNEL_HEAP_START + KERNEL_HEAP_SIZE)
#define GFT_AREA_SIZE			(4 * M)
/* area for vfs-nodes */
#define VFSNODE_AREA			(GFT_AREA + GFT_AREA_SIZE)
#define VFSNODE_AREA_SIZE		(4 * M)
/* area for sll-nodes */
#define SLLNODE_AREA			(VFSNODE_AREA + VFSNODE_AREA_SIZE)
#define SLLNODE_AREA_SIZE		(32 * M)

/* for mapping some pages of foreign processes */
#define TEMP_MAP_AREA			(SLLNODE_AREA + SLLNODE_AREA_SIZE)
#define TEMP_MAP_AREA_SIZE		(PT_ENTRY_COUNT * PAGE_SIZE * 4)

/* our kernel-stack */
#define KERNEL_STACK			(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE)

/* the size of the temporary stack we use at the beginning */
#define TMP_STACK_SIZE			PAGE_SIZE

/* converts a virtual address to the page-directory-index for that address */
#define ADDR_TO_PDINDEX(addr)	((size_t)((uintptr_t)(addr) / PAGE_SIZE / PT_ENTRY_COUNT))

/* converts a virtual address to the index in the corresponding page-table */
#define ADDR_TO_PTINDEX(addr)	((size_t)(((uintptr_t)(addr) / PAGE_SIZE) % PT_ENTRY_COUNT))

/* converts pages to page-tables (how many page-tables are required for the pages?) */
#define PAGES_TO_PTS(pageCount)	(((size_t)(pageCount) + (PT_ENTRY_COUNT - 1)) / PT_ENTRY_COUNT)

/* start-address of the text */
#define TEXT_BEGIN				0x1000
/* start-address of the text in dynamic linker */
#define INTERP_TEXT_BEGIN		0x60000000

/* free area for shared memory, tls, shared libraries, ... */
#define FREE_AREA_BEGIN			0x60000000
#define FREE_AREA_END			KERNEL_AREA_V_ADDR

extern tPageDir curPDir;

/**
 * Sets the entry with index <index> to the given translation
 */
extern void tlb_set(int index,uint virt,uint phys);

#endif /* ECO32_PAGING_H_ */