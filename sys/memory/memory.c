//
//  memory.c
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2012 by Sidney Just
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

#include <system/syslog.h>
#include <libc/string.h>
#include <libc/math.h>
#include "memory.h"

// MARK: Kernel allocator
struct pm_kernel_allocation
{
	size_t pages;
	size_t bytes;	
};

void *kallocator(size_t bytes, uint32_t flags)
{
	size_t nbytes = bytes + sizeof(struct pm_kernel_allocation);
	size_t npages = pageCount(nbytes);

	uintptr_t ppage  = pm_alloc(npages);
	if(!ppage)
		return NULL;
	
	vm_address_t vpage = vm_alloc(vm_getKernelDirectory(), ppage, npages, flags);
	if(!vpage)
		return NULL;

	struct pm_kernel_allocation *allocation = (struct pm_kernel_allocation *)vpage;
	allocation->pages = npages;
	allocation->bytes = nbytes;

	return (void *)(vpage + sizeof(struct pm_kernel_allocation));
}


void *kalloc(size_t bytes)
{
	return kallocator(bytes, VM_FLAGS_KERNEL);
}

void *ualloc(size_t bytes)
{
	return kallocator(bytes, VM_FLAGS_USERLAND);
}

void kfree(void *ptr)
{
	vm_address_t vpage = (vm_address_t)ptr;
	vpage -= sizeof(struct pm_kernel_allocation);

	uintptr_t ppage = vm_resolveVirtualAddress(vm_getKernelDirectory(), vpage);

	struct pm_kernel_allocation *allocation = (struct pm_kernel_allocation *)vpage;
	size_t pages = allocation->pages;

	pm_free(ppage, pages);
	vm_free(vm_getKernelDirectory(), vpage, pages);
}
