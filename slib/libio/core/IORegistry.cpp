//
//  IORegistry.cpp
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

#include "IORegistry.h"

#if __KERNEL
#include <kern/kprintf.h>
#endif

namespace IO
{
	__IODefineIOCoreMeta(RegistryEntry, Object)

#if __KERNEL
	static RegistryEntry *_rootRegistryEntry;

	void RegistryEntry::InitialWakeUp(MetaClass *meta)
	{
		if(meta == RegistryEntry::GetMetaClass())
		{
			_rootRegistryEntry = RegistryEntry::Alloc()->Init();
		}

		Object::InitialWakeUp(meta);
	}

	RegistryEntry *RegistryEntry::GetRootEntry()
	{
		return _rootRegistryEntry;
	}

#else
	extern "C" void *__libio_getIORootRegistry();

	void RegistryEntry::InitialWakeUp(__unused MetaClass *meta)
	{
		if(meta == RegistryEntry::GetMetaClass())
		{
			// The kernel runs the InitialWakeUp, so if it's called here, something went wrong
			panic("RegistryEntry::InitialWakeUp()");
		}

		Object::InitialWakeUp(meta);
	}

	RegistryEntry *RegistryEntry::GetRootEntry()
	{
		return reinterpret_cast<RegistryEntry *>(__libio_getIORootRegistry());
	}

#endif


	RegistryEntry *RegistryEntry::Init()
	{
		if(!Object::Init())
			return nullptr;

		_parent = nullptr;
		_children = Array::Alloc()->Init();

		return this;
	}

	void RegistryEntry::Dealloc()
	{
		_children->Release();

		Object::Dealloc();
	}


	void RegistryEntry::AttachChild(RegistryEntry *entry)
	{
		if(entry->_parent)
		{
			if(entry->_parent == this)
				return;

			entry->DetachFromParent();
		}

		_children->AddObject(entry);
		entry->AttachToParent(this);
	}

	void RegistryEntry::AttachToParent(RegistryEntry *parent)
	{
		_parent = parent;
	}

	void RegistryEntry::DetachFromParent()
	{
		if(_parent)
		{
			_parent->__DetachChild(this);
		}
	}

	void RegistryEntry::__DetachChild(RegistryEntry *child)
	{
		child->_parent = nullptr;
		_children->RemoveObject(child);
	}

	RegistryEntry *RegistryEntry::GetParent() const
	{
		return _parent;
	}
	Array *RegistryEntry::GetChildren() const
	{
		return _children;
	}
}
