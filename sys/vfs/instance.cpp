//
//  instance.cpp
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2014 by Sidney Just
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

#include "instance.h"
#include "node.h"
#include "context.h"

namespace VFS
{
	IODefineMetaVirtual(Instance, IO::Object)

	Instance *Instance::Init(Node *rootNode)
	{
		spinlock_init(&_lock);
		
		_lastID = 0;
		_rootNode = rootNode->Retain();
		_mountpoint = nullptr;

		return this;
	}

	void Instance::Dealloc()
	{
		IO::SafeRelease(_mountpoint);
		IO::SafeRelease(_rootNode);

		IO::Object::Dealloc();
	}


	void Instance::Lock()
	{
		spinlock_lock(&_lock);
	}
	void Instance::Unlock()
	{
		spinlock_unlock(&_lock);
	}

	void Instance::CleanNode(__unused Node *node)
	{}

	uint64_t Instance::GetFreeID()
	{
		return (_lastID ++);
	}

	void Instance::SetMountpoint(Mountpoint *node)
	{
		IO::SafeRelease(_mountpoint);
		_mountpoint	= IO::SafeRetain(node);
	}

	KernReturn<OS::MmapTaskEntry *> Instance::Mmap(Context *context, Node *node, OS::MmapArgs *arguments)
	{
		// Sanity check the file size
		uint64_t minSize = static_cast<uint64_t>(arguments->offset) + arguments->length;

		if(node->GetSize() < minSize)
			return Error(KERN_INVALID_ARGUMENT);


		Error error(KERN_FAILURE);

		Sys::VM::Directory *directory = context->GetTask()->GetDirectory();

		size_t pages = VM_PAGE_COUNT(arguments->length);

		uintptr_t pmemory = 0x0;
		vm_address_t vmemory = 0x0;

		OS::MmapTaskEntry *entry = nullptr;

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
			Sys::VM::Directory::Flags vmflags = Sys::VM::TranslateMmapProtection(arguments->protection);
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
			KernReturn<size_t> result = instance->FileRead(context, node, arguments->offset, reinterpret_cast<void *>(vmemory), arguments->length);

			if(!result.IsValid())
			{
				error = result.GetError();
				goto mmapFailed;
			}
		}

		// Create mmap entry
		entry = new OS::MmapTaskEntry();
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
		entry->node = node;

		return entry;

	mmapFailed:

		if(vmemory)
			directory->Free(vmemory, pages);

		if(pmemory)
			Sys::PM::Free(pmemory, pages);

		return error;
	}
	KernReturn<size_t> Instance::Msync(Context *context, OS::MmapTaskEntry *entry, OS::MsyncArgs *arguments)
	{
		// Adjust the offset
		size_t offset = reinterpret_cast<uintptr_t>(arguments->address) - entry->vmaddress;

		// Flush it down to the file
		KernReturn<size_t> result = FileWrite(context, entry->node, entry->offset + offset, reinterpret_cast<void *>(entry->vmaddress), arguments->length);

		if(result.IsValid())
			return 0;

		return result.GetError();
	}
}
