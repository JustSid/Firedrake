//
//  ffs_node.cpp
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

#include <machine/memory/memory.h>
#include <kern/kprintf.h>
#include <libcpp/algorithm.h>
#include <vfs/context.h>
#include "ffs_node.h"

namespace FFS
{
	constexpr size_t kNodeMaxChunkSize = VM_PAGE_SIZE * 10;

	IODefineMeta(Node, VFS::Node)

	Node *Node::Init(const char *name, VFS::Instance *instance, uint64_t id)
	{
		if(!VFS::Node::Init(name, instance, VFS::Node::Type::File, id))
			return nullptr;

		_data = nullptr;
		_size = 0;
		_pages = 0;

		return this;
	}

	void Node::Dealloc()
	{
		if(_data)
			Sys::Free(_data, Sys::VM::Directory::GetKernelDirectory(), _pages);

		VFS::Node::Dealloc();
	}

	KernReturn<void> Node::AllocateMemory(size_t minimumSize)
	{
		if(!_data)
		{
			size_t pages = VM_PAGE_COUNT(minimumSize);

			_data = Sys::Alloc<char>(Sys::VM::Directory::GetKernelDirectory(), pages, kVMFlagsKernel);
			if(!_data)
				return Error(KERN_NO_MEMORY);

			_pages = pages;
			return ErrorNone;
		}

		size_t pages = VM_PAGE_COUNT(minimumSize);
		if(pages > _pages)
		{
			char *temp = Sys::Alloc<char>(Sys::VM::Directory::GetKernelDirectory(), pages, kVMFlagsKernel);
			if(!temp)
				return Error(KERN_NO_MEMORY);

			memcpy(temp, _data, _size);
			Sys::Free(_data, Sys::VM::Directory::GetKernelDirectory(), _pages);

			_data  = temp;
			_pages = pages;
		}

		return ErrorNone;
	}

	KernReturn<size_t> Node::WriteData(VFS::Context *context, off_t offset, const void *data, size_t size)
	{
		size = std::min(size, kNodeMaxChunkSize);

		KernReturn<void> result;

		if((result = AllocateMemory(_size + size)).IsValid() == false)
			return result.GetError();

		if((result = Error(context->CopyDataOut(data, _data + offset, size))).IsValid() == false)
			return result.GetError();

		if(offset + size > _size)
			_size += (offset + size) - _size;

		SetSize(_size);
		return size;
	}
	KernReturn<size_t> Node::ReadData(VFS::Context *context, off_t offset, void *data, size_t size)
	{
		char *end = _data + _size;
		char *preferredEnd = _data + offset + size;

		end = std::min(end, preferredEnd);

		if(_data + offset > end)
			return 0;

		size = end - (_data + offset);
		size = std::min(size, kNodeMaxChunkSize);

		if(size > 0)
		{
			KernReturn<void> result;

			if((result = Error(context->CopyDataIn(_data + offset, data, size))).IsValid() == false)
				return result.GetError();
		}

		return size;
	}
}
