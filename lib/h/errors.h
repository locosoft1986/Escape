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

#ifndef ERRORS_H_
#define ERRORS_H_

/* error-codes */
#define ERR_FILE_IN_USE				-1
#define ERR_NO_FREE_FD				-2
#define	ERR_MAX_PROC_FDS			-3
#define ERR_VFS_NODE_NOT_FOUND		-4
#define ERR_INVALID_SYSC_ARGS		-5
#define ERR_INVALID_FD				-6
#define ERR_INVALID_FILE			-7
#define ERR_NO_READ_PERM			-8
#define ERR_NO_WRITE_PERM			-9
#define ERR_INV_SERVICE_NAME		-10
#define ERR_NOT_ENOUGH_MEM			-11
#define ERR_SERVICE_EXISTS			-12
#define ERR_PROC_DUP_SERVICE		-13
#define ERR_PROC_DUP_SERVICE_USE	-14
#define ERR_SERVICE_NOT_IN_USE		-15
#define ERR_NOT_OWN_SERVICE			-16
#define ERR_IO_MAP_RANGE_RESERVED	-17
#define ERR_IOMAP_NOT_PRESENT		-18
#define ERR_INTRPT_LISTENER_MSGLEN	-19
#define ERR_INVALID_IRQ_NUMBER		-20
#define ERR_IRQ_LISTENER_MISSING	-21
#define ERR_NO_CLIENT_WAITING		-22
#define ERR_FS_NOT_FOUND			-23
#define ERR_INVALID_SIGNAL			-24
#define ERR_INVALID_PID				-25
#define ERR_NO_DIRECTORY			-26
#define ERR_PATH_NOT_FOUND			-27
#define ERR_FS_READ_FAILED			-28
#define ERR_INVALID_PATH			-29
#define ERR_INVALID_NODENO			-30
#define ERR_SERVUSE_SEEK			-31
#define ERR_MAX_PROCS_REACHED		-32
#define ERR_NODE_EXISTS				-33
#define ERR_INVALID_ELF_BINARY		-34
#define ERR_SHARED_MEM_EXISTS		-35
#define ERR_SHARED_MEM_NAME			-36
#define ERR_SHARED_MEM_INVALID		-37
#define ERR_LOCK_NOT_FOUND			-38
#define ERR_INVALID_TID				-39
#define ERR_UNSUPPORTED_OPERATION	-40
#define ERR_FS_INODE_ALLOC			-41
#define ERR_FS_WRITE_FAILED			-42
#define ERR_FS_INODE_NOT_FOUND		-43

#if IN_KERNEL
#define ERR_REAL_PATH				-200
#endif

#endif /* ERRORS_H_ */
