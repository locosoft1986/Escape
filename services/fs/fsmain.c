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
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/service.h>
#include <esc/proc.h>
#include <esc/heap.h>
#include <esc/debug.h>
#include <messages.h>
#include <errors.h>
#include <stdlib.h>
#include <string.h>
#include <fsinterface.h>

#include "ext2/ext2.h"
#include "ext2/path.h"
#include "ext2/inode.h"
#include "ext2/inodecache.h"
#include "ext2/file.h"

static sMsg msg;
static sExt2 ext2;

int main(void) {
	tFD fd;
	tMsgId mid;
	s32 size;
	tServ id,client;

	if(!ext2_init(&ext2)) {
		unregService(id);
		return EXIT_FAILURE;
	}

	/* register service */
	id = regService("fs",SERV_FS);
	if(id < 0) {
		printe("Unable to register service 'fs'");
		return EXIT_FAILURE;
	}

	while(1) {
		fd = getClient(&id,1,&client);
		if(fd < 0)
			wait(EV_CLIENT);
		else {
			while((size = receive(fd,&mid,&msg,sizeof(msg))) > 0) {
				switch(mid) {
					case MSG_FS_OPEN: {
						tInodeNo no = ext2_resolvePath(&ext2,msg.str.s1);

						/*debugf("Received an open from %d of '%s' for ",data->pid,data + 1);
						if(data->flags & IO_READ)
							debugf("READ");
						if(data->flags & IO_WRITE) {
							if(data->flags & IO_READ)
								debugf(" and ");
							debugf("WRITE");
						}
						debugf("\n");
						debugf("Path is associated with inode %d\n",no);*/

						/*ext2_icache_printStats();
						ext2_bcache_printStats();*/

						/* write response */
						msg.args.arg1 = no;
						send(fd,MSG_FS_OPEN_RESP,&msg,sizeof(msg.args));
					}
					break;

					case MSG_FS_STAT: {
						sCachedInode *cnode;
						tInodeNo no;

						no = ext2_resolvePath(&ext2,msg.str.s1);
						if(no >= 0) {
							cnode = ext2_icache_request(&ext2,no);
							if(cnode != NULL) {
								sFileInfo *info = (sFileInfo*)&(msg.data.d);
								info->accesstime = cnode->inode.accesstime;
								info->modifytime = cnode->inode.modifytime;
								info->createtime = cnode->inode.createtime;
								info->blockCount = cnode->inode.blocks;
								info->blockSize = BLOCK_SIZE(&ext2);
								info->device = 0;
								info->rdevice = 0;
								info->uid = cnode->inode.uid;
								info->gid = cnode->inode.gid;
								info->inodeNo = cnode->inodeNo;
								info->linkCount = cnode->inode.linkCount;
								info->mode = cnode->inode.mode;
								info->size = cnode->inode.size;
								msg.data.arg1 = 0;
							}
							else
								msg.data.arg1 = ERR_NOT_ENOUGH_MEM;
						}
						else
							msg.data.arg1 = no;

						/* write response */
						send(fd,MSG_FS_STAT_RESP,&msg,sizeof(msg.data));
					}
					break;

					case MSG_FS_READ: {
						tInodeNo ino = (tInodeNo)msg.args.arg1;
						u32 offset = msg.args.arg2;
						u32 count = msg.args.arg3;

						u8 *buffer = malloc(count);
						if(buffer == NULL)
							msg.args.arg1 = ERR_NOT_ENOUGH_MEM;
						else
							msg.args.arg1 = ext2_readFile(&ext2,ino,buffer,offset,count);
						send(fd,MSG_FS_READ_RESP,&msg,sizeof(msg.args));
						if(buffer) {
							send(fd,MSG_FS_READ_RESP,buffer,count);
							free(buffer);
						}

						/* read ahead
						if(count > 0)
							ext2_readFile(&ext2,data.inodeNo,NULL,data.offset + count,data.count);*/
					}
					break;

					case MSG_FS_WRITE: {
						tInodeNo ino = (tInodeNo)msg.args.arg1;
						u32 offset = msg.args.arg2;
						u32 count = msg.args.arg3;
						u8 *buffer;

						/* write to file */
						msg.args.arg1 = 0;
						buffer = malloc(count);
						if(buffer) {
							receive(fd,&mid,buffer,count);
							msg.args.arg1 = ext2_writeFile(&ext2,ino,buffer,offset,count);
							free(buffer);
						}
						/* send response */
						send(fd,MSG_FS_WRITE_RESP,&msg,sizeof(msg.args));
					}
					break;

					case MSG_FS_SYNC: {
						ext2_sync(&ext2);
					}
					break;

					case MSG_FS_CLOSE: {
						/*debugf("Closing inode %d\n",data.inodeNo);*/
					}
					break;
				}
			}
			close(fd);
		}
	}

	/* clean up */
	unregService(id);

	return EXIT_SUCCESS;
}
