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

#include <common.h>
#include <vfs/irq.h>
#include <interrupts.h>
#include <log.h>
#include <ostream.h>
#include <string.h>

const size_t InterruptsBase::IRQ_SEM_COUNT = IRQ_COUNT;
SpinLock InterruptsBase::userIrqsLock;
esc::ISList<Semaphore*> InterruptsBase::userIrqs[IRQ_SEM_COUNT];

void InterruptsBase::initVFS() {
	VFSNode *node = NULL;
	sassert(VFSNode::request("/sys/irq",NULL,&node,NULL,VFS_READ,0) == 0);
	for(int i = 0; i < IRQ_COUNT; ++i)
		VFSNode::release(createObj<VFSIRQ>(KERNEL_PID,node,i,0500));
}

int InterruptsBase::attachSem(Semaphore *sem,size_t irq,const char *name,
		uint64_t *msiaddr,uint32_t *msival) {
	LockGuard<SpinLock> g(&userIrqsLock);
	assert(irq < IRQ_SEM_COUNT);
	if(userIrqs[irq].length() == 0) {
		int res = Interrupts::installHandler(irq,name);
		if(res < 0)
			return res;
	}
	Interrupts::getMSIAttr(irq,msiaddr,msival);
	userIrqs[irq].append(sem);
	return 0;
}

void InterruptsBase::detachSem(Semaphore *sem,size_t irq) {
	LockGuard<SpinLock> g(&userIrqsLock);
	assert(irq < IRQ_SEM_COUNT);
	userIrqs[irq].remove(sem);
	if(userIrqs[irq].length() == 0)
		Interrupts::uninstallHandler(irq);
}

bool InterruptsBase::fireIrq(size_t irq) {
	LockGuard<SpinLock> g(&userIrqsLock);
	assert(irq < IRQ_SEM_COUNT);
	esc::ISList<Semaphore*> *list = userIrqs + irq;
	for(auto it = list->begin(); it != list->end(); ++it)
		(*it)->up();
	return list->length() > 0;
}

size_t InterruptsBase::getCount() {
	ulong total = 0;
	for(size_t i = 0; i < IRQ_COUNT; ++i)
		total += intrptList[i].count;
	return total;
}

void InterruptsBase::print(OStream &os) {
	for(size_t i = 0; i < IRQ_COUNT; ++i) {
		if(strcmp(intrptList[i].name,"??") != 0)
			printIRQ(i, os);
	}
}

void InterruptsBase::printIRQ(int irq,OStream &os) {
	if(irq >= IRQ_COUNT)
		return;

	const Interrupt *i = intrptList + irq;
	os.writef("%3d %-20s %lu\n",irq,i->name,i->count);
}
