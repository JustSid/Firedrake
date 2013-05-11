//
//  context.c
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2013 by Sidney Just
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

#include <errno.h>
#include <libc/string.h>
#include <scheduler/scheduler.h>
#include "context.h"

extern vfs_context_t *vfs_kernelContext;
extern vfs_node_t *vfs_rootNode;

vfs_context_t *vfs_getKernelContext()
{
	return vfs_kernelContext;
}

vfs_context_t *vfs_getCurrentContext()
{
	process_t *process = process_getCurrentProcess();
	return process->context;
}

vfs_context_t *vfs_contextCreate(vm_page_directory_t directory)
{
	vfs_context_t *context = halloc(NULL, sizeof(vfs_context_t));
	if(context)
	{
		context->chdir = vfs_rootNode;
		context->pdirectory = directory;
	}

	return context;
}

void vfs_contextDelete(vfs_context_t *context)
{
	hfree(NULL, context);
}

void vfs_contextChangeDirectory(vfs_context_t *context, vfs_node_t *node)
{
	assert(node);
	context->chdir = node;
}

bool vfs_contextCopyDataOut(vfs_context_t *context, const void *data, size_t size, void *target, int *errno)
{
	if(!data || !target || size == 0)
	{
		*errno = EINVAL;
		return false;
	}

	if(context->pdirectory == vm_getKernelDirectory())
	{
		memcpy(target, data, size);
		return true;
	}
	else
	{
		vm_address_t temp = (vm_address_t)data;
		vm_address_t page = round4kDown(temp);

		size_t pages  = pageCount(size);
		size_t offset = temp - page; 

		uintptr_t physical = vm_resolveVirtualAddress(context->pdirectory, page);
		if(!physical)
		{
			*errno = EINVAL;
			return false;
		}

		vm_address_t tvirtual = vm_alloc(vm_getKernelDirectory(), physical, pages, VM_FLAGS_KERNEL);
		if(!tvirtual)
		{
			*errno = ENOMEM;
			return false;
		}

		void *mapped = (void *)(tvirtual + offset);
		memcpy(target, mapped, size);

		vm_free(vm_getKernelDirectory(), tvirtual, pages);
		return true;
	}
}

bool vfs_contextCopyDataIn(vfs_context_t *context, const void *data, size_t size, void *target, int *errno)
{
	if(!data || !target || size == 0)
	{
		*errno = EINVAL;
		return false;
	}

	if(context->pdirectory == vm_getKernelDirectory())
	{
		memcpy(target, data, size);
		return true;
	}
	else
	{
		vm_address_t temp = (vm_address_t)target;
		vm_address_t page = round4kDown(temp);

		size_t pages  = pageCount(size);
		size_t offset = temp - page; 

		uintptr_t physical = vm_resolveVirtualAddress(context->pdirectory, page);
		if(!physical)
		{
			*errno = EINVAL;
			return false;
		}

		vm_address_t tvirtual = vm_alloc(vm_getKernelDirectory(), physical, pages, VM_FLAGS_KERNEL);
		if(!tvirtual)
		{
			*errno = ENOMEM;
			return false;
		}

		void *mapped = (void *)(tvirtual + offset);
		memcpy(mapped, data, size);

		vm_free(vm_getKernelDirectory(), tvirtual, pages);
		return true;
	}
}
