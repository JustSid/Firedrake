//
//  syscall_mmap.cpp
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2016 by Sidney Just
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

#ifndef _SYSCALL_MMAP_H_
#define _SYSCALL_MMAP_H_

#include <prefix.h>
#include <libc/stdint.h>
#include <libc/sys/mman.h>
#include <kern/kern_return.h>
#include <os/scheduler/thread.h>
#include <vfs/file.h>
#include <vfs/node.h>

namespace OS
{
	struct MmapArgs
	{
		void *address;
		size_t length;
		int protection;
		int flags;
		int fd;
		off_t offset;
	} __attribute__((packed));

	struct MsyncArgs
	{
		void *address;
		size_t length;
		int flags;
	} __attribute__((packed));

	struct MmapTaskEntry
	{
		MmapTaskEntry(VFS::Node *tnode) :
			node(tnode),
			taskEntry(this)
		{
			IO::SafeRetain(node);
		}

		~MmapTaskEntry()
		{
			IO::SafeRelease(node);
		}

		uintptr_t phaddress;
		vm_address_t vmaddress;
		size_t pages;

		int protection;
		int flags;

		off_t offset;

		VFS::Node *node;
		std::intrusive_list<MmapTaskEntry>::member taskEntry;
	};

	KernReturn<uint32_t> Syscall_mmap(OS::Thread *thread, MmapArgs *arguments);
	KernReturn<uint32_t> Syscall_msync(OS::Thread *thread, MsyncArgs *arguments);
}

#endif /* _SYSCALL_MMAP_H_ */
