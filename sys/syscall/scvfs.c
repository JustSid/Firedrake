//
//  scvfs.c
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2013 by Sidney Just
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
//  documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
//  the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
//  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include <scheduler/scheduler.h>
#include <system/syslog.h>
#include <system/panic.h>
#include <vfs/vfs.h>
#include "syscall.h"

uint32_t _sc_open(__unused uint32_t *esp, uint32_t *uesp, int *errno)
{
	const char *tpath = *(const char **)(uesp + 0);
	int flags = *(int *)(uesp + 1);

	vm_address_t virtual;

	char *path = sc_mapProcessMemory(tpath, &virtual, 2, errno);
	int fd = vfs_open(path, flags, errno);

	vm_free(vm_getKernelDirectory(), virtual, 2);
	return (uint32_t)fd;
}

uint32_t _sc_close(__unused uint32_t *esp, uint32_t *uesp, __unused int *errno)
{
	int fd = *(int *)(uesp + 0);
	int result = vfs_close(fd);
	return (uint32_t)result;
}


uint32_t _sc_read(__unused uint32_t *esp, uint32_t *uesp, int *errno)
{
	int fd = *(int *)(uesp + 0);
	void *data = *(void **)(uesp + 1);
	size_t size = *(size_t *)(uesp + 2);

	size_t result = vfs_read(fd, data, size, errno);
	return (uint32_t)result;
}

uint32_t _sc_write(__unused uint32_t *esp, uint32_t *uesp, int *errno)
{
	int fd = *(int *)(uesp + 0);
	void *data = *(void **)(uesp + 1);
	size_t size = *(size_t *)(uesp + 2);

	size_t result = vfs_write(fd, data, size, errno);
	return (uint32_t)result;
}

uint32_t _sc_seek(__unused uint32_t *esp, uint32_t *uesp, int *errno)
{
	int fd = *(int *)(uesp + 0);
	off_t offset = *(off_t *)(uesp + 1);
	int whence = *(int *)(uesp + 2);

	size_t result = vfs_seek(fd, offset, whence, errno);
	return (uint32_t)result;
}

uint32_t _sc_readdir(__unused uint32_t *esp, uint32_t *uesp, int *errno)
{
	int fd = *(int *)(uesp + 0);
	void *ptr = *(void **)(uesp + 1);
	uint32_t count = *(int *)(uesp + 2);

	off_t result = vfs_readDir(fd, ptr, count, errno);
	return (uint32_t)result;
}


uint32_t _sc_mkdir(__unused uint32_t *esp, uint32_t *uesp, int *errno)
{
	const char *tpath = *(const char **)(uesp + 0);

	vm_address_t virtual;
	char *path = sc_mapProcessMemory(tpath, &virtual, 2, errno);

	bool result = vfs_mkdir(path, errno);
	vm_free(vm_getKernelDirectory(), virtual, 2);

	return result ? 0 : (size_t)-1;
}

uint32_t _sc_remove(__unused uint32_t *esp, uint32_t *uesp, int *errno)
{
	const char *tpath = *(const char **)(uesp + 0);

	vm_address_t virtual;
	char *path = sc_mapProcessMemory(tpath, &virtual, 2, errno);

	bool result = vfs_remove(path, errno);
	vm_free(vm_getKernelDirectory(), virtual, 2);

	return result ? 0 : (size_t)-1;
}

uint32_t _sc_move(__unused uint32_t *esp, uint32_t *uesp, int *errno)
{
	const char *tpath1 = *(const char **)(uesp + 0);
	const char *tpath2 = *(const char **)(uesp + 1);

	vm_address_t virtual1, virtual2;
	char *path1 = sc_mapProcessMemory(tpath1, &virtual1, 2, errno);
	char *path2 = sc_mapProcessMemory(tpath2, &virtual2, 2, errno);

	bool result = vfs_move(path1, path2, errno);

	vm_free(vm_getKernelDirectory(), virtual1, 2);
	vm_free(vm_getKernelDirectory(), virtual2, 2);

	return result ? 0 : (size_t)-1;
}

uint32_t _sc_stat(__unused uint32_t *esp, uint32_t *uesp, int *errno)
{
	const char *tpath = *(const char **)(uesp + 0);
	vfs_stat_t *stat = *(vfs_stat_t **)(uesp + 1);

	vm_address_t virtual;
	char *path = sc_mapProcessMemory(tpath, &virtual, 2, errno);

	bool result = vfs_stat(path, stat, errno);
	vm_free(vm_getKernelDirectory(), virtual, 2);

	return result ? 0 : (size_t)-1;
}


void _sc_vfsInit()
{
	sc_setSyscallHandler(SYS_OPEN, _sc_open);
	sc_setSyscallHandler(SYS_CLOSE, _sc_close);
	sc_setSyscallHandler(SYS_READ, _sc_read);
	sc_setSyscallHandler(SYS_WRITE, _sc_write);
	sc_setSyscallHandler(SYS_SEEK, _sc_seek);
	sc_setSyscallHandler(SYS_DIRREAD, _sc_readdir);
	sc_setSyscallHandler(SYS_MKDIR, _sc_mkdir);
	sc_setSyscallHandler(SYS_REMOVE, _sc_remove);
	sc_setSyscallHandler(SYS_MOVE, _sc_move);
	sc_setSyscallHandler(SYS_STAT, _sc_stat);
}
