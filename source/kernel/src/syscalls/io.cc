/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <sys/common.h>
#include <sys/mem/paging.h>
#include <sys/mem/vmm.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/task/fd.h>
#include <sys/syscalls/io.h>
#include <sys/syscalls.h>
#include <esc/messages.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

int sysc_open(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	uint flags = (uint)SYSC_ARG2(stack);
	sFile *file;
	int res,fd;
	pid_t pid = t->getProc()->getPid();
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	/* check flags */
	flags &= VFS_USER_FLAGS;
	if((flags & (VFS_READ | VFS_WRITE | VFS_MSGS)) == 0)
		SYSC_ERROR(stack,-EINVAL);

	/* open the path */
	res = vfs_openPath(pid,flags,abspath,&file);
	if(res < 0)
		SYSC_ERROR(stack,res);

	/* assoc fd with file */
	fd = fd_assoc(file);
	if(fd < 0) {
		vfs_closeFile(pid,file);
		SYSC_ERROR(stack,fd);
	}
	SYSC_RET1(stack,fd);
}

int sysc_fcntl(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	uint cmd = SYSC_ARG2(stack);
	int arg = (int)SYSC_ARG3(stack);
	pid_t pid = t->getProc()->getPid();
	sFile *file;
	int res;

	/* get file */
	file = fd_request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	res = vfs_fcntl(pid,file,cmd,arg);
	fd_release(file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_pipe(Thread *t,sIntrptStackFrame *stack) {
	int *readFd = (int*)SYSC_ARG1(stack);
	int *writeFd = (int*)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();
	sFile *readFile,*writeFile;
	int kreadFd,kwriteFd;
	int res;

	/* make sure that the pointers point to userspace */
	if(!paging_isInUserSpace((uintptr_t)readFd,sizeof(int)) ||
			!paging_isInUserSpace((uintptr_t)writeFd,sizeof(int)))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_openPipe(pid,&readFile,&writeFile);
	if(res < 0)
		SYSC_ERROR(stack,res);

	/* assoc fd with read-file */
	kreadFd = fd_assoc(readFile);
	if(kreadFd < 0) {
		vfs_closeFile(pid,readFile);
		vfs_closeFile(pid,writeFile);
		SYSC_ERROR(stack,kreadFd);
	}

	/* assoc fd with write-file */
	kwriteFd = fd_assoc(writeFile);
	if(kwriteFd < 0) {
		fd_unassoc(kreadFd);
		vfs_closeFile(pid,readFile);
		vfs_closeFile(pid,writeFile);
		SYSC_ERROR(stack,kwriteFd);
	}

	/* now copy the fds to userspace; this may fail, but we have file-descriptors for the files,
	 * so the resources will be free'd in any case. */
	*readFd = kreadFd;
	*writeFd = kwriteFd;

	/* yay, we're done! :) */
	SYSC_RET1(stack,res);
}

int sysc_stat(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	sFileInfo *info = (sFileInfo*)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();
	int res;
	if(!paging_isInUserSpace((uintptr_t)info,sizeof(sFileInfo)))
		SYSC_ERROR(stack,-EFAULT);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_stat(pid,abspath,info);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int sysc_fstat(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	sFileInfo *info = (sFileInfo*)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();
	sFile *file;
	int res;
	if(!paging_isInUserSpace((uintptr_t)info,sizeof(sFileInfo)))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	file = fd_request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);
	/* get info */
	res = vfs_fstat(pid,file,info);
	fd_release(file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int sysc_chmod(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	mode_t mode = (mode_t)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();
	int res;
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_chmod(pid,abspath,mode);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int sysc_chown(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	uid_t uid = (uid_t)SYSC_ARG2(stack);
	gid_t gid = (gid_t)SYSC_ARG3(stack);
	pid_t pid = t->getProc()->getPid();
	int res;
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_chown(pid,abspath,uid,gid);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int sysc_tell(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	off_t *pos = (off_t*)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();
	sFile *file;
	if(!paging_isInUserSpace((uintptr_t)pos,sizeof(off_t)))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	file = fd_request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	/* this may fail, but we're requested the file, so it will be released on our termination */
	*pos = vfs_tell(pid,file);
	fd_release(file);
	SYSC_RET1(stack,0);
}

int sysc_seek(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	off_t offset = (off_t)SYSC_ARG2(stack);
	uint whence = SYSC_ARG3(stack);
	pid_t pid = t->getProc()->getPid();
	sFile *file;
	off_t res;

	if(whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
		SYSC_ERROR(stack,-EINVAL);

	/* get file */
	file = fd_request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	res = vfs_seek(pid,file,offset,whence);
	fd_release(file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_read(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	void *buffer = (void*)SYSC_ARG2(stack);
	size_t count = SYSC_ARG3(stack);
	pid_t pid = t->getProc()->getPid();
	ssize_t readBytes;
	sFile *file;

	/* validate count and buffer */
	if(count == 0)
		SYSC_ERROR(stack,-EINVAL);
	if(!paging_isInUserSpace((uintptr_t)buffer,count))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	file = fd_request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	/* read */
	readBytes = vfs_readFile(pid,file,buffer,count);
	fd_release(file);
	if(readBytes < 0)
		SYSC_ERROR(stack,readBytes);
	SYSC_RET1(stack,readBytes);
}

int sysc_write(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	const void *buffer = (const void*)SYSC_ARG2(stack);
	size_t count = SYSC_ARG3(stack);
	pid_t pid = t->getProc()->getPid();
	ssize_t writtenBytes;
	sFile *file;

	/* validate count and buffer */
	if(count == 0)
		SYSC_ERROR(stack,-EINVAL);
	if(!paging_isInUserSpace((uintptr_t)buffer,count))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	file = fd_request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	/* read */
	writtenBytes = vfs_writeFile(pid,file,buffer,count);
	fd_release(file);
	if(writtenBytes < 0)
		SYSC_ERROR(stack,writtenBytes);
	SYSC_RET1(stack,writtenBytes);
}

int sysc_send(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	msgid_t id = (msgid_t)SYSC_ARG2(stack);
	const void *data = (const void*)SYSC_ARG3(stack);
	size_t size = SYSC_ARG4(stack);
	pid_t pid = t->getProc()->getPid();
	sFile *file;
	ssize_t res;
	if(!paging_isInUserSpace((uintptr_t)data,size))
		SYSC_ERROR(stack,-EFAULT);
	/* can't be sent by user-programs */
	if(IS_DEVICE_MSG(id))
		SYSC_ERROR(stack,-EPERM);

	/* get file */
	file = fd_request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	/* send msg */
	res = vfs_sendMsg(pid,file,id,data,size,NULL,0);
	fd_release(file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_receive(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	msgid_t *id = (msgid_t*)SYSC_ARG2(stack);
	void *data = (void*)SYSC_ARG3(stack);
	size_t size = SYSC_ARG4(stack);
	pid_t pid = t->getProc()->getPid();
	sFile *file;
	ssize_t res;
	if(!paging_isInUserSpace((uintptr_t)data,size))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	file = fd_request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	/* send msg */
	res = vfs_receiveMsg(pid,file,id,data,size,false);
	fd_release(file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_dup(A_UNUSED Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	int res;

	res = fd_dup(fd);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_redirect(A_UNUSED Thread *t,sIntrptStackFrame *stack) {
	int src = (int)SYSC_ARG1(stack);
	int dst = (int)SYSC_ARG2(stack);
	int err = fd_redirect(src,dst);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,err);
}

int sysc_close(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	pid_t pid = t->getProc()->getPid();

	sFile *file = fd_request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	/* close file */
	fd_unassoc(fd);
	if(!vfs_closeFile(pid,file))
		fd_release(file);
	else
		Thread::remFileUsage(file);
	SYSC_RET1(stack,0);
}

int sysc_sync(Thread *t,sIntrptStackFrame *stack) {
	int res;
	pid_t pid = t->getProc()->getPid();
	res = vfs_sync(pid);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_link(Thread *t,sIntrptStackFrame *stack) {
	char oldabs[MAX_PATH_LEN + 1];
	char newabs[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = t->getProc()->getPid();
	const char *oldPath = (const char*)SYSC_ARG1(stack);
	const char *newPath = (const char*)SYSC_ARG2(stack);
	if(!sysc_absolutize_path(oldabs,sizeof(oldabs),oldPath))
		SYSC_ERROR(stack,-EFAULT);
	if(!sysc_absolutize_path(newabs,sizeof(newabs),newPath))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_link(pid,oldabs,newabs);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_unlink(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = t->getProc()->getPid();
	const char *path = (const char*)SYSC_ARG1(stack);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_unlink(pid,abspath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_mkdir(A_UNUSED Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = Proc::getRunning();
	const char *path = (const char*)SYSC_ARG1(stack);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_mkdir(pid,abspath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_rmdir(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = t->getProc()->getPid();
	const char *path = (const char*)SYSC_ARG1(stack);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_rmdir(pid,abspath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_mount(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	char absdev[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = t->getProc()->getPid();
	const char *device = (const char*)SYSC_ARG1(stack);
	const char *path = (const char*)SYSC_ARG2(stack);
	uint type = (uint)SYSC_ARG3(stack);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);
	if(!sysc_absolutize_path(absdev,sizeof(absdev),device))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_mount(pid,absdev,abspath,type);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_unmount(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = t->getProc()->getPid();
	const char *path = (const char*)SYSC_ARG1(stack);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_unmount(pid,abspath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
