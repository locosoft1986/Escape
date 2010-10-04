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

#include <esc/common.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/ports.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/mem.h>
#include <signal.h>
#include <stdio.h>
#include "ata.h"
#include "controller.h"

/* port-bases */
#define PORTBASE_PRIMARY			0x1F0
#define PORTBASE_SECONDARY			0x170

#define IDE_CTRL_CLASS				0x01
#define IDE_CTRL_SUBCLASS			0x01
#define IDE_CTRL_BAR				4

#define BMR_SEC_OFFSET				0x8

#define DMA_BUF_SIZE				(64 * 1024)

static void ctrl_intrptHandler(int sig);
static bool ctrl_isBusResponding(sATAController* ctrl);

static sPCIDevice ideCtrl;
static sATAController ctrls[2];

void ctrl_init(bool useDma) {
	ssize_t i,j;
	int res;
	tFD fd;

	/* get ide-controller from pci */
	fd = open("/dev/pci",IO_READ | IO_WRITE);
	if(fd < 0)
		error("Unable to open '/dev/pci'");
	res = vrecvMsgData(fd,MSG_PCI_GET_BY_CLASS,&ideCtrl,sizeof(sPCIDevice),2,
			IDE_CTRL_CLASS,IDE_CTRL_SUBCLASS);
	if(res < 0)
		error("Unable to find IDE-controller (%d:%d)",IDE_CTRL_CLASS,IDE_CTRL_SUBCLASS);
	ATA_LOG("Found IDE-controller (%d.%d.%d): vendorId %x, deviceId %x, rev %x",
			ideCtrl.bus,ideCtrl.dev,ideCtrl.func,ideCtrl.vendorId,ideCtrl.deviceId,ideCtrl.revId);
	close(fd);

	ctrls[0].id = DEVICE_PRIMARY;
	ctrls[0].irq = SIG_INTRPT_ATA1;
	ctrls[0].portBase = PORTBASE_PRIMARY;

	ctrls[1].id = DEVICE_SECONDARY;
	ctrls[1].irq = SIG_INTRPT_ATA2;
	ctrls[1].portBase = PORTBASE_SECONDARY;

	/* request io-ports for bus-mastering */
	if(useDma && ideCtrl.bars[IDE_CTRL_BAR].addr) {
		if(requestIOPorts(ideCtrl.bars[IDE_CTRL_BAR].addr,ideCtrl.bars[IDE_CTRL_BAR].size) < 0) {
			error("Unable to request ATA-ports %d .. %d",ideCtrl.bars[IDE_CTRL_BAR].addr,
					ideCtrl.bars[IDE_CTRL_BAR].addr + ideCtrl.bars[IDE_CTRL_BAR].size - 1);
		}
	}

	for(i = 0; i < 2; i++) {
		ATA_PR2("Initializing controller %d",ctrls[i].id);
		ctrls[i].useIrq = true;
		ctrls[i].useDma = false;
		ctrls[i].gotIrq = false;

		/* request ports */
		/* for some reason virtualbox requires an additional port (9 instead of 8). Otherwise
		 * we are not able to access port (ATA_REG_BASE_PRIMARY + 7). */
		if(requestIOPorts(ctrls[i].portBase,9) < 0)
			error("Unable to request ATA-ports %d .. %d",ctrls[i].portBase,ctrls[i].portBase + 8);
		if(requestIOPort(ctrls[i].portBase + ATA_REG_CONTROL) < 0)
			error("Unable to request ATA-port %d",ctrls[i].portBase + ATA_REG_CONTROL);

		/* check if the bus is empty */
		ATA_PR1("Checking if bus %d is floating",ctrls[i].id);
		if(!ctrl_isBusResponding(ctrls + i)) {
			ATA_LOG("Bus %d is floating",ctrls[i].id);
			continue;
		}

		/* set interrupt-handler */
		if(setSigHandler(ctrls[i].irq,ctrl_intrptHandler) < 0)
			error("Unable to announce sig-handler ctrls %d (%d)",ctrls[i].id,ctrls[i].irq);

		/* init DMA */
		ctrls[i].bmrBase = ideCtrl.bars[IDE_CTRL_BAR].addr;
		if(useDma && ctrls[i].bmrBase) {
			ctrls[i].bmrBase += i * BMR_SEC_OFFSET;
			/* allocate memory for PRDT and buffer */
			ctrls[i].dma_prdt_virt = allocPhysical((uintptr_t*)&ctrls[i].dma_prdt_phys,8,4096);
			if(!ctrls[i].dma_prdt_virt)
				error("Unable to allocate PRDT for controller %d",ctrls[i].id);
			ctrls[i].dma_buf_virt = allocPhysical((uintptr_t*)&ctrls[i].dma_buf_phys,
					DMA_BUF_SIZE,DMA_BUF_SIZE);
			if(!ctrls[i].dma_buf_virt)
				error("Unable to allocate dma-buffer for controller %d",ctrls[i].id);
			ctrls[i].useDma = true;
		}

		/* init attached devices; begin with slave */
		for(j = 1; j >= 0; j--) {
			ctrls[i].devices[j].present = false;
			ctrls[i].devices[j].id = i * 2 + j;
			ctrls[i].devices[j].ctrl = ctrls + i;
			device_init(ctrls[i].devices + j);
		}
	}
	ATA_PR2("All controller initialized");
}

sATADevice *ctrl_getDevice(uchar id) {
	return ctrls[id / 2].devices + id % 2;
}

sATAController *ctrl_getCtrl(uchar id) {
	return ctrls + id;
}

void ctrl_outbmrb(sATAController *ctrl,uint16_t reg,uint8_t value) {
	outByte(ctrl->bmrBase + reg,value);
}

void ctrl_outbmrl(sATAController *ctrl,uint16_t reg,uint32_t value) {
	outDWord(ctrl->bmrBase + reg,value);
}

uint8_t ctrl_inbmrb(sATAController *ctrl,uint16_t reg) {
	return inByte(ctrl->bmrBase + reg);
}

void ctrl_outb(sATAController *ctrl,uint16_t reg,uint8_t value) {
	outByte(ctrl->portBase + reg,value);
}

void ctrl_outwords(sATAController *ctrl,uint16_t reg,const uint16_t *buf,size_t count) {
	/* TODO according to the wiki, we shouldn't use that because the device needs a small delay
	 * between the words */
	outWordStr(ctrl->portBase + reg,buf,count);
}

uint8_t ctrl_inb(sATAController *ctrl,uint16_t reg) {
	return inByte(ctrl->portBase + reg);
}

void ctrl_inwords(sATAController *ctrl,uint16_t reg,uint16_t *buf,size_t count) {
	inWordStr(ctrl->portBase + reg,buf,count);
}

void ctrl_softReset(sATAController *ctrl) {
	uint8_t status;
	ctrl_outb(ctrl,ATA_REG_CONTROL,4);
	ctrl_outb(ctrl,ATA_REG_CONTROL,0);
	ctrl_wait(ctrl);
	do {
		status = ctrl_inb(ctrl,ATA_REG_STATUS);
	}
	while((status & (CMD_ST_BUSY | CMD_ST_READY)) != CMD_ST_READY);
}

void ctrl_resetIrq(sATAController *ctrl) {
	ctrl->gotIrq = false;
}

void ctrl_waitIntrpt(sATAController *ctrl) {
	tTime time = 0;
	if(!ctrl->useIrq)
		return;
	while(!ctrl->gotIrq) {
		/* if we reached the timeout, it seems that waiting for interrupts does not work for
		 * this controller. so disable it */
		if(time > IRQ_TIMEOUT) {
			ATA_LOG("Controller %d: IRQ-Timeout reached; stopping to use interrupts",ctrl->id);
			ctrl->useIrq = false;
			return;
		}
		sleep(IRQ_POLL_INTERVAL);
		time += IRQ_POLL_INTERVAL;
	}
}

int ctrl_waitUntil(sATAController *ctrl,tTime timeout,tTime sleepTime,uint8_t set,uint8_t unset) {
	tTime time = 0;
	while(time < timeout) {
		uint8_t status = ctrl_inb(ctrl,ATA_REG_STATUS);
		if(status & CMD_ST_ERROR)
			return ctrl_inb(ctrl,ATA_REG_ERROR);
		if((status & set) == set && !(status & unset))
			return 0;
		ATA_PR1("Status %#x",status);
		if(sleepTime) {
			sleep(sleepTime);
			time += sleepTime;
		}
		else
			time++;
	}
	return -1;
}

static void ctrl_intrptHandler(int sig) {
	size_t i;
	for(i = 0; i < 2; i++) {
		if(ctrls[i].irq == sig) {
			ctrls[i].gotIrq = true;
			break;
		}
	}
}

void ctrl_wait(sATAController *ctrl) {
	inByte(ctrl->portBase + ATA_REG_STATUS);
	inByte(ctrl->portBase + ATA_REG_STATUS);
	inByte(ctrl->portBase + ATA_REG_STATUS);
	inByte(ctrl->portBase + ATA_REG_STATUS);
}

static bool ctrl_isBusResponding(sATAController* ctrl) {
	ssize_t i;
	for(i = 1; i >= 0; i--) {
		/* begin with slave. master should respond if there is no slave */
		outByte(ctrl->portBase + ATA_REG_DRIVE_SELECT,i << 4);
		ctrl_wait(ctrl);

		/* write some arbitrary values to some registers */
		outByte(ctrl->portBase + ATA_REG_ADDRESS1,0xF1);
		outByte(ctrl->portBase + ATA_REG_ADDRESS2,0xF2);
		outByte(ctrl->portBase + ATA_REG_ADDRESS3,0xF3);

		/* if we can read them back, the bus is present */
		if(inByte(ctrl->portBase + ATA_REG_ADDRESS1) == 0xF1 &&
			inByte(ctrl->portBase + ATA_REG_ADDRESS2) == 0xF2 &&
			inByte(ctrl->portBase + ATA_REG_ADDRESS3) == 0xF3)
			return true;
	}
	return false;
}
