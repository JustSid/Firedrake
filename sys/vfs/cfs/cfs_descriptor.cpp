//
//  cfs_descriptor.cpp
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
#include <vfs/instance.h>
#include "cfs_descriptor.h"
#include "cfs_instance.h"

namespace CFS
{
	IODefineMeta(Descriptor, VFS::Descriptor)

	Descriptor *Descriptor::Init()
	{
		if(!VFS::Descriptor::Init("cfs", Flags::Persistent))
			return nullptr;

		KernReturn<void> result;

		if((result = Register()).IsValid() == false)
		{
			kprintf("Failed to register FFS, reason %d\n", result.GetError().GetCode());
			return nullptr;
		}

		return this;
	}

	KernReturn<VFS::Instance *> Descriptor::CreateInstance()
	{
		return Instance::Alloc()->Init();
	}
	
	void Descriptor::DestroyInstance(__unused VFS::Instance *instance)
	{}
}
