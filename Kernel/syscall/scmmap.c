//
//  scmmap.c
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

#include <scheduler/scheduler.h>
#include <system/syslog.h>
#include <memory/memory.h>
#include <libc/string.h>
#include "syscall.h"

#define PROT_NONE   0x00
#define PROT_READ   0x01
#define PROT_WRITE  0x02
#define PROT_EXEC	0x04

#define MAP_SHARED    0x0001 // Not supported yet
#define MAP_PRIVATE   0x0002

#define MAP_ANONYMOUS 	0x0004 // Must be specified because of lack of file descriptors
#define MAP_FAILED		-1

// mmap() signature:
// void *mmap(void *addr, size_t length, int prot, int flags, int fd, uint32_t offset)

uint32_t _sc_mmap(uint32_t *esp, uint32_t *uesp, int *errno)
{
	process_t *process = process_getCurrentProcess();

	uintptr_t address = *((uintptr_t *)(uesp + 0));
	size_t length     = *((size_t *)(uesp + 1));
	int protection    = *((int *)(uesp + 2));
	int flags         = *((int *)(uesp + 3));
	int filed         = *((int *)(uesp + 4));
	uint32_t offset   = *((int *)(uesp + 5));

	spinlock_lock(&process->mmapLock);
	mmap_description_t *description = list_addBack(process->mappings);

	if(!description)
	{
		spinlock_unlock(&process->mmapLock);
		return MAP_FAILED;
	}

	if((flags & MAP_PRIVATE) && (flags & MAP_SHARED))
		goto mmapFailed;

	if((flags & MAP_PRIVATE) && (flags & MAP_ANONYMOUS))
	{
		// Check if address and length are on an 4k aligned
		if((address % 4096) != 0 || (length % 4096) != 0)
		{
			*errno = EINVAL;
			return MAP_FAILED;
		}

		// Get the right virtual memory flags for the requested protection bits
		uint32_t vmflags = VM_PAGETABLEFLAG_PRESENT;

		if(!(protection & PROT_NONE))
			vmflags |= VM_FLAGS_USERLAND;

		if(!(protection & PROT_READ) && (protection & PROT_WRITE))
			vmflags |= VM_PAGETABLEFLAG_WRITEABLE;


		size_t pages      = pageCount(length);
		uintptr_t pmemory = pm_alloc(pages);

		if(!pmemory)
		{
			*errno = ENOMEM;
			goto mmapFailed;
		}


		vm_address_t vmemory = 0x0;
		if(address != 0x0)
		{
			// todo: This could be done more elegant, but does the trick for the meantime...
			vmemory = vm_allocLimit(process->pdirectory, pmemory, (vm_address_t)address, pages, vmflags);

			if(!vmemory)
				vmemory = vm_alloc(process->pdirectory, pmemory, pages, vmflags);
		}
		else
		{
			vmemory = vm_alloc(process->pdirectory, pmemory, pages, vmflags);
		}

		if(!vmemory)
		{
			*errno = ENOMEM;

			pm_free(pmemory, pages);
			goto mmapFailed;
		}


		// Map the memory also in kernel address space and initialize it with zeroes
		void *memory = (void *)vm_alloc(vm_getKernelDirectory(), pmemory, pages, VM_FLAGS_KERNEL);
		if(!memory)
		{
			*errno = ENOMEM;
			
			vm_free(process->pdirectory, vmemory, pages);
			pm_free(pmemory, pages);

			goto mmapFailed;
		}

		memset(memory, 0, pages * VM_PAGE_SIZE);
		vm_free(vm_getKernelDirectory(), (vm_address_t)memory, pages);

		// Add the description
		description->vaddress = vmemory;
		description->paddress = pmemory;

		return vmemory;
	}


mmapFailed:
	list_remove(process->mappings, description);
	spinlock_unlock(&process->mmapLock);
	return MAP_FAILED;
}



void _sc_mmapInit()
{
	sc_setSyscallHandler(SYS_MMAP, _sc_mmap);
}
