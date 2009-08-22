/**
 * $Id$
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

#include <types.h>
#include <assert.h>
#include <ringbuffer.h>

#ifdef IN_KERNEL
#	include <kheap.h>
#	include <video.h>
/* for util_panic (vassert) */
#	include <util.h>
#	define rbprintf	vid_printf
#	define free(x)		kheap_free(x)
#	define malloc(x)	kheap_alloc(x)
#else
#	include <esc/heap.h>
#	include <esc/debug.h>
/* for exit (vassert) */
#	include <esc/proc.h>
#	include <esc/fileio.h>
#	define rbprintf debugf
#endif

typedef struct {
	u32 count;		/* current number of elements that can be read */
	u32 readPos;	/* current read-pos */
	u32 writePos;	/* current write-pos */
	u32 eSize;		/* size of one entry */
	u32 eMax;		/* max number of entries in <data> */
	u8 flags;
	char data[];
} sIRingBuf;

sRingBuf *rb_create(u32 eSize,u32 eCount,u8 flags) {
	sIRingBuf *rb;

	vassert(eSize > 0,"eSize == 0");
	vassert(eCount > 0,"eCount == 0");

	rb = (sIRingBuf*)malloc(sizeof(sIRingBuf) + eSize * eCount);
	if(rb == NULL)
		return NULL;
	rb->eSize = eSize;
	rb->eMax = eCount;
	rb->count = 0;
	rb->readPos = 0;
	rb->writePos = 0;
	rb->flags = flags;
	return (sRingBuf*)rb;
}

void rb_destroy(sRingBuf *r) {
	vassert(r != NULL,"r == NULL");
	free(r);
}

u32 rb_length(sRingBuf *r) {
	sIRingBuf *rb = (sIRingBuf*)r;
	return rb == NULL ? 0 : rb->count;
}

bool rb_write(sRingBuf *r,const void *e) {
	bool maxReached;
	sIRingBuf *rb = (sIRingBuf*)r;
	vassert(r != NULL,"r == NULL");
	vassert(e != NULL,"e == NULL");

	maxReached = rb->count >= rb->eMax;
	if(!(rb->flags & RB_OVERWRITE) && maxReached)
		return false;
	memcpy(rb->data + rb->eSize * rb->writePos,e,rb->eSize);
	rb->writePos = (rb->writePos + 1) % rb->eMax;
	if((rb->flags & RB_OVERWRITE) && maxReached)
		rb->readPos = (rb->readPos + 1) % rb->eMax;
	else
		rb->count++;
	return true;
}

bool rb_read(sRingBuf *r,void *e) {
	sIRingBuf *rb = (sIRingBuf*)r;
	vassert(r != NULL,"r == NULL");
	vassert(e != NULL,"e == NULL");

	if(rb->count == 0)
		return false;
	memcpy(e,rb->data + rb->eSize * rb->readPos,rb->eSize);
	rb->readPos = (rb->readPos + 1) % rb->eMax;
	rb->count--;
	return true;
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void rb_dbg_print(sRingBuf *r) {
	sIRingBuf *rb = (sIRingBuf*)r;
	u32 i,c,s;
	char *addr;
	vassert(r != NULL,"r == NULL");

	rbprintf("RingBuf [cnt=%d, rpos=%d, wpos=%d, emax=%d, esize=%d]:",rb->count,rb->readPos,
			rb->writePos,rb->eMax,rb->eSize);
	i = rb->readPos;
	for(c = 0, i = rb->readPos; c < rb->count; c++, i = (i + 1) % rb->eMax) {
		if(c % 6 == 0)
			rbprintf("\n\t");
		rbprintf("0x");
		/* take care of little-endian */
		addr = rb->data + (i + 1) * rb->eSize - 1;
		for(s = 0; s < rb->eSize; s++)
			rbprintf("%02x",*addr--);
		rbprintf(" ");
	}
	rbprintf("\n");
}

#endif
