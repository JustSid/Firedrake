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
#include "vfs_syscall.h"

namespace VFS
{
	KernReturn<uint32_t> Syscall_VFSOpen(OS::Thread *thread, VFSOpenArgs *arguments)
	{
		OS::SyscallScopedMapping pathMapping(thread->GetTask(), arguments->path, MAXNAME);

		const char *path = pathMapping.GetMemory<const char>();

		KernReturn<int> fd = Open(thread->GetTask()->GetVFSContext(), path, arguments->flags);

		if(!fd.IsValid())
			return fd.GetError();

		return fd.Get();
	}

	KernReturn<uint32_t> Syscall_VFSClose(OS::Thread *thread, VFSCloseArgs *arguments)
	{
		KernReturn<void> result = Close(thread->GetTask()->GetVFSContext(), arguments->fd);

		if(!result.IsValid())
			return result.GetError();

		return 0;
	}

	KernReturn<uint32_t> Syscall_VFSWrite(OS::Thread *thread, VFSWriteArgs *arguments)
	{
		KernReturn<size_t> result = Write(thread->GetTask()->GetVFSContext(), arguments->fd, arguments->data, arguments->size);

		if(!result.IsValid())
			return result.GetError();

		return result.Get();
	}

	KernReturn<uint32_t> Syscall_VFSRead(OS::Thread *thread, VFSReadArgs *arguments)
	{
		KernReturn<size_t> result = Read(thread->GetTask()->GetVFSContext(), arguments->fd, arguments->data, arguments->size);

		if(!result.IsValid())
			return result.GetError();

		return result.Get();
	}

	KernReturn<uint32_t> Syscall_VFSSeek(OS::Thread *thread, VFSSeekArgs *arguments)
	{
		KernReturn<off_t> result = Seek(thread->GetTask()->GetVFSContext(), arguments->fd, arguments->offset, arguments->whence);

		if(!result.IsValid())
			return result.GetError();

		return static_cast<uint32_t>(result.Get());
	}

	KernReturn<uint32_t> Syscall_VFSIoctl(OS::Thread *thread, VFSIoctlArgs *arguments)
	{
		KernReturn<void> result = Ioctl(thread->GetTask()->GetVFSContext(), arguments->fd, arguments->request, arguments->arg);

		if(!result.IsValid())
			return result.GetError();

		return 0;
	}
}
