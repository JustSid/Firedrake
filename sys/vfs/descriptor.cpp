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

namespace VFS
{
	void RegisterDescriptor(Descriptor *descriptor);
	void UnregisterDescriptor(Descriptor *descriptor);

	Descriptor::Descriptor(const char *name, Flags flags) :
		_name(name),
		_flags(flags),
		_lock(SPINLOCK_INIT),
		_registered(false)
	{}

	Descriptor::~Descriptor()
	{

	}

	void Descriptor::AddInstance(Instance *instance)
	{
		_instances.push_back(instance);
	}
	bool Descriptor::RemoveInstance(Instance *instance)
	{
		auto iterator = std::find(_instances.begin(), _instances.end(), instance);
		if(iterator != _instances.end())
		{
			_instances.erase(iterator);
			return true;
		}
		
		return false;
	}

	kern_return_t Descriptor::Register()
	{
		Lock();
		if(_registered)
		{
			kprintf("Descriptor already registered!\n");
			Unlock();

			return KERN_RESOURCE_IN_USE;
		}

		_registered = true;

		RegisterDescriptor(this);
		Unlock();

		return KERN_SUCCESS;
	}
	kern_return_t Descriptor::Unregister()
	{
		Lock();

		if(!_registered)
		{
			kprintf("Descriptor not registered!\n");
			Unlock();

			return KERN_RESOURCES_MISSING;
		}

		_registered = false;

		UnregisterDescriptor(this);
		Unlock();

		return KERN_SUCCESS;
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
