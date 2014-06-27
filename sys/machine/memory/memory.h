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

namespace mm
{
	template<class T=void>
	T *alloc(vm::directory *dir, size_t pages, uint32_t flags)
	{
		uintptr_t pmemory    = 0;
		vm_address_t vmemory = 0;

		kern_return_t result;

		result = pm::alloc(pmemory, pages);
		if(result != KERN_SUCCESS)
			goto alloc_failed;

		result = dir->alloc(vmemory, pmemory, pages, flags);
		if(result != KERN_SUCCESS)
			goto alloc_failed;

		return reinterpret_cast<T *>(vmemory);

	alloc_failed:
		if(pmemory)
			pm::free(pmemory, pages);

		if(vmemory)
			dir->free(vmemory, pages);

		return nullptr;
	}

	template<class T>
	void free(T *ptr, vm::directory *dir, size_t pages)
	{
		kern_return_t result;
		uintptr_t pmemory;

		result = dir->resolve_virtual_address(pmemory, reinterpret_cast<vm_address_t>(ptr));
		if(result == KERN_SUCCESS)
		{
			dir->free(reinterpret_cast<vm_address_t>(ptr), pages);
			pm::free(pmemory, pages);
		}
	}

	template<class T>
	T *Allocate()
	{
		void *buffer = kalloc(sizeof(T));
		if(!buffer)
			return nullptr;

		T *result = new(buffer) T();
	}
}

#endif /* _MEMORY_H_ */
