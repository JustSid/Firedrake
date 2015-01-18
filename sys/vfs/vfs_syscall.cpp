//
//  vfs_syscall.cpp
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2015 by Sidney Just
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

#include <os/syscall/syscall.h>
#include <kern/kprintf.h>
#include "vfs.h"

namespace VFS
{
	struct VFSOpenArgs
	{
		const char *path;
		int flags;
	} __attribute__((packed));

	struct VFSCloseArgs
	{
		int fd;
	} __attribute__((packed));

	struct VFSWriteArgs
	{
		int fd;
		const void *data;
		size_t size;
	} __attribute__((packed));

	struct VFSReadArgs
	{
		int fd;
		void *data;
		size_t size;
	} __attribute__((packed));

	struct VFSSeekArgs
	{
		int fd;
		off_t offset;
		int whence;
	} __attribute__((packed));

	KernReturn<uint32_t> Syscall_VFSOpen(__unused uint32_t &esp, void *args)
	{
		VFSOpenArgs *arguments = (VFSOpenArgs *)args;
		OS::SyscallScopedMapping pathMapping(arguments->path, MAXNAME);

		const char *path = pathMapping.GetMemory<const char>();

		KernReturn<int> fd = Open(Context::GetActiveContext(), path, arguments->flags);

		if(!fd.IsValid())
			return fd.GetError();

		return fd.Get();
	}

	KernReturn<uint32_t> Syscall_VFSClose(__unused uint32_t &esp, void *args)
	{
		VFSCloseArgs *arguments = (VFSCloseArgs *)args;
		KernReturn<void> result = Close(Context::GetActiveContext(), arguments->fd);

		if(!result.IsValid())
			return result.GetError();

		return 0;
	}

	KernReturn<uint32_t> Syscall_VFSWrite(__unused uint32_t &esp, void *args)
	{
		VFSWriteArgs *arguments = (VFSWriteArgs *)args;
		KernReturn<size_t> result = Write(Context::GetActiveContext(), arguments->fd, arguments->data, arguments->size);

		if(!result.IsValid())
			return result.GetError();

		return result.Get();
	}

	KernReturn<uint32_t> Syscall_VFSRead(__unused uint32_t &esp, void *args)
	{
		VFSReadArgs *arguments = (VFSReadArgs *)args;
		KernReturn<size_t> result = Read(Context::GetActiveContext(), arguments->fd, arguments->data, arguments->size);

		if(!result.IsValid())
			return result.GetError();

		return result.Get();
	}

	KernReturn<uint32_t> Syscall_VFSSeek(__unused uint32_t &esp, void *args)
	{
		VFSSeekArgs *arguments = (VFSSeekArgs *)args;
		KernReturn<off_t> result = Seek(Context::GetActiveContext(), arguments->fd, arguments->offset, arguments->whence);

		if(!result.IsValid())
			return result.GetError();

		return static_cast<uint32_t>(result.Get());
	}



	KernReturn<void> Syscall_VFSInit()
	{
		OS::SetSyscallHandler(SYS_Open, &Syscall_VFSOpen, sizeof(VFSOpenArgs), true);
		OS::SetSyscallHandler(SYS_Close, &Syscall_VFSClose, sizeof(VFSCloseArgs), true);

		OS::SetSyscallHandler(SYS_Write, &Syscall_VFSWrite, sizeof(VFSWriteArgs), true);
		OS::SetSyscallHandler(SYS_Read, &Syscall_VFSRead, sizeof(VFSReadArgs), true);
		OS::SetSyscallHandler(SYS_Seek, &Syscall_VFSSeek, sizeof(VFSSeekArgs), true);

		return ErrorNone;
	}
}
