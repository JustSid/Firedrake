//
//  context.cpp
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

#include <libc/string.h>
#include <kern/scheduler/scheduler.h>
#include "context.h"
#include "vfs.h"

namespace VFS
{
	Context::Context(Sys::VM::Directory *directory, Node *current) :
		_lock(SPINLOCK_INIT),
		_directory(directory),
		_currentDir(current),
		_root(VFS::GetRootNode())
	{
		_currentDir->Retain();
		_root->Retain();
	}

	Context::~Context()
	{
		_currentDir->Release();
		_root->Release();
	}


	KernReturn<void> Context::SetCurrentDir(Node *directory)
	{
		if(!directory)
			return Error(KERN_INVALID_ARGUMENT);

		spinlock_lock(&_lock);

		_currentDir->Release();
		_currentDir = directory;
		_currentDir->Retain();

		spinlock_unlock(&_lock);

		return ErrorNone;
	}
	KernReturn<void> Context::SetRootDir(Node *directory)
	{
		if(!directory)
			return Error(KERN_INVALID_ARGUMENT);

		spinlock_lock(&_lock);

		_root->Release();
		_root = directory;
		_root->Retain();

		spinlock_unlock(&_lock);

		return ErrorNone;
	}

	Node *Context::GetCurrentDir()
	{
		spinlock_lock(&_lock);

		Node *node = _currentDir;
		node->Retain();

		spinlock_unlock(&_lock);

		return node;
	}
	Node *Context::GetRootDir()
	{
		spinlock_lock(&_lock);

		Node *node = _root;
		node->Retain();

		spinlock_unlock(&_lock);

		return node;
	}


	KernReturn<void> Context::CopyDataOut(const void *data, void *target, size_t length)
	{
		if(!data || !target || length == 0)
			return Error(KERN_INVALID_ARGUMENT);

		Sys::VM::Directory *kernelDir = Sys::VM::Directory::GetKernelDirectory();
		if(_directory == kernelDir)
		{
			memcpy(target, data, length);
			return ErrorNone;
		}
		else
		{
			vm_address_t temp = reinterpret_cast<vm_address_t>(data);
			vm_address_t page = VM_PAGE_ALIGN_DOWN(temp);

			size_t pages  = VM_PAGE_COUNT(length);
			size_t offset = temp - page; 

			KernReturn<uintptr_t> physical;

			if((physical = _directory->ResolveAddress(page)).IsValid() == false)
				return physical.GetError();
			
			KernReturn<vm_address_t> mapping;

			if((mapping = kernelDir->Alloc(physical, pages, kVMFlagsKernel)).IsValid() == false)
				return mapping.GetError();

			memcpy(target, reinterpret_cast<void *>(mapping + offset), length);
			kernelDir->Free(mapping, pages);
		}

		return ErrorNone;
	}

	KernReturn<void> Context::CopyDataIn(const void *data, void *target, size_t length)
	{
		if(!data || !target || length == 0)
			return Error(KERN_INVALID_ARGUMENT);

		Sys::VM::Directory *kernelDir = Sys::VM::Directory::GetKernelDirectory();
		if(_directory == kernelDir)
		{
			memcpy(target, data, length);
			return ErrorNone;
		}
		else
		{
			vm_address_t temp = reinterpret_cast<vm_address_t>(target);
			vm_address_t page = VM_PAGE_ALIGN_DOWN(temp);

			size_t pages  = VM_PAGE_COUNT(length);
			size_t offset = temp - page; 

			KernReturn<uintptr_t> physical;

			if((physical = _directory->ResolveAddress(page)).IsValid() == false)
				return physical.GetError();
			
			KernReturn<vm_address_t> mapping;

			if((mapping = kernelDir->Alloc(physical, pages, kVMFlagsKernel)).IsValid() == false)
				return mapping.GetError();

			memcpy(target, reinterpret_cast<void *>(mapping + offset), length);
			kernelDir->Free(mapping, pages);
		}

		return ErrorNone;
	}



	Context *Context::GetKernelContext()
	{
		static Context *_kernelContext = nullptr;
		if(__expect_false(!_kernelContext))
		{
			Core::Task *task = Core::Scheduler::GetActiveTask();

			_kernelContext = new Context(Sys::VM::Directory::GetKernelDirectory(), VFS::GetRootNode());
			task->_context = _kernelContext;
		}

		return _kernelContext;
	}

	Context *Context::GetActiveContext()
	{
		Core::Task *task = Core::Scheduler::GetActiveTask();
		return task->_context;
	}
}
