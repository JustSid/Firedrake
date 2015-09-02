//
//  vfs_syscall.h
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

#include <prefix.h>
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

	struct VFSIoctlArgs
	{
		int fd;
		uint32_t request;
		void *arg;
	};

	KernReturn<uint32_t> Syscall_VFSOpen(OS::Thread *thread, VFSOpenArgs *arguments);
	KernReturn<uint32_t> Syscall_VFSClose(OS::Thread *thread, VFSCloseArgs *arguments);
	KernReturn<uint32_t> Syscall_VFSWrite(OS::Thread *thread, VFSWriteArgs *arguments);
	KernReturn<uint32_t> Syscall_VFSRead(OS::Thread *thread, VFSReadArgs *arguments);
	KernReturn<uint32_t> Syscall_VFSSeek(OS::Thread *thread, VFSSeekArgs *arguments);
	KernReturn<uint32_t> Syscall_VFSIoctl(OS::Thread *thread, VFSIoctlArgs *arguments);
}