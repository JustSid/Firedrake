//
//  descriptor.cpp
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

#include <kern/kprintf.h>
#include "descriptor.h"
#include "instance.h"

namespace VFS
{
	IODefineMetaVirtual(Descriptor, IO::Object)

	void RegisterDescriptor(Descriptor *descriptor);
	void UnregisterDescriptor(Descriptor *descriptor);

	Descriptor *Descriptor::Init(const char *name, Flags flags)
	{
		if(!Object::Init())
			return nullptr;

		spinlock_init(&_lock);

		_name  = IO::String::Alloc()->InitWithCString(name);
		_flags = flags;
		_registered = false;
		_instances  = IO::Array::Alloc()->Init();

		return this;
	}

	void Descriptor::Dealloc()
	{
		_instances->Release();
		_name->Release();

		Object::Dealloc();
	}


	void Descriptor::AddInstance(Instance *instance)
	{
		_instances->AddObject(instance);
	}
	bool Descriptor::RemoveInstance(Instance *instance)
	{
		size_t index = _instances->GetIndexOfObject(instance);
		if(index != IO::kNotFound)
			return false;

		_instances->RemoveObjectAtIndex(index);
		return true;
	}

	KernReturn<void> Descriptor::Register()
	{
		Lock();
		if(_registered)
		{
			kprintf("Descriptor already registered!\n");
			Unlock();

			return Error(KERN_RESOURCE_IN_USE);
		}

		_registered = true;

		RegisterDescriptor(this);
		Unlock();

		return ErrorNone;
	}
	KernReturn<void> Descriptor::Unregister()
	{
		Lock();

		if(!_registered)
		{
			kprintf("Descriptor not registered!\n");
			Unlock();

			return Error(KERN_RESOURCES_MISSING);
		}

		_registered = false;

		UnregisterDescriptor(this);
		Unlock();

		return ErrorNone;
	}

	void Descriptor::Lock()
	{
		spinlock_lock(&_lock);
	}
	void Descriptor::Unlock()
	{
		spinlock_unlock(&_lock);
	}
}
