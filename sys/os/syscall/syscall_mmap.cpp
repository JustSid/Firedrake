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

#include <machine/memory/physical.h>
#include <os/scheduler/task.h>
#include <vfs/vfs.h>
#include <vfs/file.h>
#include "syscall_mmap.h"

namespace OS
{
	static inline Sys::VM::Directory::Flags mmapTranslateProtection(int protection)
	{
		Sys::VM::Directory::Flags vmflags = Sys::VM::Directory::Flags::Present;

		if(!(protection & PROT_NONE))
			vmflags |= Sys::VM::Directory::Flags::Userspace;

		if((protection & PROT_WRITE))
			vmflags |= Sys::VM::Directory::Flags::Writeable;

		return vmflags;
	}

	KernReturn<MmapTaskEntry *> mmapAnonymous(OS::Task *task, MmapArgs *arguments)
	{
		Error error(KERN_FAILURE);
		Sys::VM::Directory *directory = task->GetDirectory();

		size_t pages = VM_PAGE_COUNT(arguments->length);

		uintptr_t pmemory = 0x0;
		vm_address_t vmemory = 0x0;

		MmapTaskEntry *entry = nullptr;

		// Find some physical storage
		{
			KernReturn<uintptr_t> result = Sys::PM::Alloc(pages);
			if(!result.IsValid())
			{
				error = result.GetError();
				goto mmapFailed;
			}

			pmemory = result.Get();
		}

		{
			Sys::VM::Directory::Flags  vmflags = mmapTranslateProtection(arguments->protection);
			KernReturn<vm_address_t> result = directory->Alloc(pmemory, pages, vmflags);

			if(!result.IsValid())
			{
				error = result.GetError();
				goto mmapFailed;
			}

			vmemory = result.Get();
		}

		// Zero out the memory
		// TODO: Use page fault handler to clean pages when they are used


		// Create mmap entry
		entry = new MmapTaskEntry();
		if(!entry)
		{
			error = Error(KERN_NO_MEMORY);
			goto mmapFailed;
		}

		entry->phaddress = pmemory;
		entry->vmaddress = vmemory;
		entry->protection = arguments->protection;
		entry->pages = pages;
		entry->flags = arguments->flags;
		entry->offset = arguments->offset;
		entry->file = nullptr;

		return entry;

	mmapFailed:
		if(vmemory)
			directory->Free(vmemory, pages);

		if(pmemory)
			Sys::PM::Free(pmemory, pages);

		return error;
	}

	KernReturn<MmapTaskEntry *> mmapFile(OS::Task *task, MmapArgs *arguments)
	{
		task->Lock();

		int fd = arguments->fd;
		VFS::File *file = task->GetFileForDescriptor(fd);

		if(!file)
		{
			task->Unlock();
			return Error(KERN_INVALID_ARGUMENT);
		}

		VFS::Node *node = file->GetNode();
		if(node->IsDirectory())
		{
			task->Unlock();
			return Error(KERN_INVALID_ARGUMENT);
		}

		// Sanity check the file size
		uint64_t minSize = static_cast<uint64_t>(arguments->offset) + arguments->length;

		if(node->GetSize() < minSize)
		{
			task->Unlock();
			return Error(KERN_INVALID_ARGUMENT);
		}

		Error error(KERN_FAILURE);

		Sys::VM::Directory *directory = task->GetDirectory();
		VFS::Context *context = task->GetVFSContext();

		size_t pages = VM_PAGE_COUNT(arguments->length);

		uintptr_t pmemory = 0x0;
		vm_address_t vmemory = 0x0;

		MmapTaskEntry *entry = nullptr;

		// Find some physical storage
		{
			KernReturn<uintptr_t> result = Sys::PM::Alloc(pages);
			if(!result.IsValid())
			{
				error = result.GetError();
				goto mmapFailed;
			}

			pmemory = result.Get();
		}

		{
			Sys::VM::Directory::Flags vmflags = mmapTranslateProtection(arguments->protection);
			KernReturn<vm_address_t> result = directory->Alloc(pmemory, pages, vmflags);

			if(!result.IsValid())
			{
				error = result.GetError();
				goto mmapFailed;
			}

			vmemory = result.Get();
		}

		// Read the file into the buffer
		{
			VFS::Instance *instance = node->GetInstance();
			KernReturn<size_t> result = instance->FileRead(context, file, arguments->offset, reinterpret_cast<void *>(vmemory), arguments->length);

			if(!result.IsValid())
			{
				error = result.GetError();
				goto mmapFailed;
			}
		}

		task->Unlock();

		// Create mmap entry
		entry = new MmapTaskEntry();
		if(!entry)
		{
			error = Error(KERN_NO_MEMORY);
			goto mmapFailed;
		}

		entry->phaddress = pmemory;
		entry->vmaddress = vmemory;
		entry->protection = arguments->protection;
		entry->pages = pages;
		entry->flags = arguments->flags;
		entry->offset = arguments->offset;
		entry->file = file;

		return entry;

	mmapFailed:
		task->Unlock();

		if(vmemory)
			directory->Free(vmemory, pages);

		if(pmemory)
			Sys::PM::Free(pmemory, pages);

		return error;
	}

	KernReturn<uint32_t> Syscall_mmap(OS::Thread *thread, MmapArgs *arguments)
	{
		uintptr_t address = reinterpret_cast<uintptr_t>(arguments->address);
		int flags = arguments->flags;
		size_t length = arguments->length;

		// Basic sanity checks
		if((flags & MAP_PRIVATE) && (flags & MAP_SHARED))
			return Error(KERN_INVALID_ARGUMENT);

		// It's either PROT_NONE or anything else
		if(flags & PROT_NONE && flags != PROT_NONE)
			return Error(KERN_INVALID_ARGUMENT);

		OS::Task *task = thread->GetTask();

		if(flags & MAP_ANONYMOUS)
		{
			if((address && (address % VM_PAGE_SIZE) != 0) || (length % VM_PAGE_SIZE) != 0 || length == 0)
				return Error(KERN_INVALID_ARGUMENT);

			KernReturn<MmapTaskEntry *> result = mmapAnonymous(task, arguments);
			if(!result.IsValid())
				return result.GetError();

			MmapTaskEntry *entry = result.Get();

			task->Lock();
			task->mmapList.push_front(entry->taskEntry);
			task->Unlock();

			return entry->vmaddress;
		}
		else
		{
			if((address && (address % VM_PAGE_SIZE) != 0) || (arguments->offset && (arguments->offset % VM_PAGE_SIZE) != 0) || length == 0)
				return Error(KERN_INVALID_ARGUMENT);

			KernReturn<MmapTaskEntry *> result = mmapFile(task, arguments);
			if(!result.IsValid())
				return result.GetError();

			MmapTaskEntry *entry = result.Get();

			task->Lock();
			task->mmapList.push_front(entry->taskEntry);
			task->Unlock();

			return entry->vmaddress;
		}
	}

	KernReturn<uint32_t> Syscall_msync(OS::Thread *thread, MsyncArgs *arguments)
	{
		vm_address_t address = reinterpret_cast<vm_address_t >(arguments->address);

		// Find the mmap entry for the address
		std::intrusive_list<MmapTaskEntry>::member *member = thread->GetTask()->mmapList.head();
		while(member)
		{
			MmapTaskEntry *entry = member->get();

			if(entry->vmaddress <= address && entry->vmaddress + (entry->pages * VM_PAGE_SIZE) >= address + arguments->length)
			{
				if(entry->flags & MAP_PRIVATE || entry->flags & MAP_ANONYMOUS)
					return 0; // Nothing to do for us

				// Adjust the offset
				size_t offset = address - entry->vmaddress;

				// Flush it down to the file
				VFS::Instance *instance = entry->file->GetNode()->GetInstance();
				KernReturn<size_t> result = instance->FileWrite(thread->GetTask()->GetVFSContext(), entry->file, entry->offset + offset, reinterpret_cast<void *>(entry->vmaddress), arguments->length);

				if(!result.IsValid())
					return 0;

				return result.GetError();
			}

			member = member->next();
		}

		return Error(KERN_FAILURE);
	}
}
