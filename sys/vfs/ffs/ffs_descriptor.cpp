//
//  ffs_descriptor.cpp
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

#include <libcpp/new.h>
#include <vfs/instance.h>
#include "ffs_descriptor.h"
#include "ffs_instance.h"

namespace FFS
{
	Descriptor::Descriptor() :
		VFS::Descriptor("ffs", Flags::Persistent)
	{}

	kern_return_t Descriptor::CreateAndRegister(VFS::Descriptor *&descriptor)
	{
		void *buffer = kalloc(sizeof(Descriptor));
		if(!buffer)
			return KERN_NO_MEMORY;

		kern_return_t result;
		descriptor = new(buffer) Descriptor();

		if((result = descriptor->Register()) != KERN_SUCCESS)
		{
			delete descriptor;
			return result;
		}

		return KERN_SUCCESS;
	}

	kern_return_t Descriptor::CreateInstance(VFS::Instance *&instance)
	{
		void *buffer = kalloc(sizeof(Instance));
		if(!buffer)
			return KERN_NO_MEMORY;

		instance = new(buffer) Instance();
		return KERN_SUCCESS;
	}
	
	void Descriptor::DestroyInstance(__unused VFS::Instance *instance)
	{}
}
