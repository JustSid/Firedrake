//
//  memory.h
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

#ifndef _MEMORY_H_
#define _MEMORY_H_

// This header also works as umbrella header for the other memory headers
#include "physical.h"
#include "virtual.h"
#include "heap.h"

#include <kern/kalloc.h>
#include <libcpp/type_traits.h>

namespace Sys
{
	template<class T=void>
	T *Alloc(VM::Directory *dir, size_t pages, uint32_t flags)
	{
		KernReturn<uintptr_t> pmemory;
		KernReturn<vm_address_t> vmemory;

		pmemory = PM::Alloc(pages);
		if(pmemory.IsValid() == false)
			goto alloc_failed;

		vmemory = dir->Alloc(pmemory, pages, flags);
		if(vmemory.IsValid() == false)
			goto alloc_failed;

		return reinterpret_cast<T *>(vmemory.Get());

	alloc_failed:
		if(pmemory)
			PM::Free(pmemory, pages);

		if(vmemory)
			dir->Free(vmemory, pages);

		return nullptr;
	}

	template<class T>
	void Free(T *ptr, VM::Directory *dir, size_t pages)
	{
		KernReturn<uintptr_t> pmemory;

		pmemory = dir->ResolveAddress(reinterpret_cast<vm_address_t>(ptr));
		if(pmemory.IsValid())
		{
			dir->Free(reinterpret_cast<vm_address_t>(ptr), pages);
			PM::Free(pmemory, pages);
		}
	}
}

#endif /* _MEMORY_H_ */
