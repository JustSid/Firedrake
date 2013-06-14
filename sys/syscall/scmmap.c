//
//  scmmap.c
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

#include <libc/assert.h>
#include <system/panic.h>
#include <system/syslog.h>
#include <libc/string.h>

#include "scmmap.h"
#include "syscall.h"

static inline uint32_t mmap_vmflagsForProtectionFlags(int protection)
{
	uint32_t vmflags = VM_PAGETABLEFLAG_PRESENT;

	if(!(protection & PROT_NONE))
		vmflags |= VM_PAGETABLEFLAG_USERSPACE;

	if((protection & PROT_WRITE))
		vmflags |= VM_PAGETABLEFLAG_WRITEABLE;

	return vmflags;
}

bool mmap_copyMappings(process_t *target, process_t *source)
{
	process_lock(source);
	process_block(source);

	size_t pages = atree_count(source->mappings);
	uintptr_t pmemory = pm_alloc(pages);

	mmap_description_t *description;
	mmap_description_t *targetDescription = halloc(NULL, pages * sizeof(mmap_description_t));
	iterator_t *iterator = atree_iterator(source->mappings);

	assert(targetDescription && iterator && pmemory);

	vm_address_t targetPage = vm_alloc(vm_getKernelDirectory(), pmemory, 2, VM_FLAGS_KERNEL);
	vm_address_t sourcePage = targetPage + VM_PAGE_SIZE;

	assert(targetPage);
	
	while((description = iterator_nextObject(iterator)))
	{
		vm_mapPage__noLock(vm_getKernelDirectory(), description->paddress, sourcePage, VM_FLAGS_KERNEL);
		vm_mapPage__noLock(vm_getKernelDirectory(), pmemory, targetPage, VM_FLAGS_KERNEL);

		memcpy((void *)targetPage, (void *)sourcePage, VM_PAGE_SIZE);

		targetDescription->paddress = pmemory;
		targetDescription->vaddress = description->vaddress;
		targetDescription->protection = description->protection;

		uint32_t vmflags = mmap_vmflagsForProtectionFlags(targetDescription->protection);
		vm_mapPage__noLock(target->pdirectory, targetDescription->paddress, targetDescription->vaddress, vmflags);

		atree_insert(target->mappings, targetDescription, (void *)(targetDescription->vaddress));

		pmemory += VM_PAGE_SIZE;
		targetDescription ++;
	}

	iterator_destroy(iterator);

	vm_free(vm_getKernelDirectory(), targetPage, 2);

	process_unblock(source);
	process_unlock(source);

	return true;
}

void mmap_destroyMappings(process_t *process)
{
	mmap_description_t *description;
	iterator_t *iterator = atree_iterator(process->mappings);

	assert(iterator);

	while((description = iterator_nextObject(iterator)))
	{
		vm_mapPage__noLock(process->pdirectory, 0x0, description->vaddress, 0);
		pm_free(description->paddress, 1);
	}

	iterator_destroy(iterator);
	atree_destroy(process->mappings);

	process->mappings = atree_create(mmap_atreeLookup);
}

int mmap_atreeLookup(void *key1, void *key2)
{
	vm_address_t t1 = (vm_address_t)key1;
	vm_address_t t2 = (vm_address_t)key2;	

	if(t1 > t2)
		return kCompareGreaterThan;

	if(t1 < t2)
		return kCompareLesserThan;

	return kCompareEqualTo;
}

// mmap() signature:
// void *mmap(void *addr, size_t length, int prot, int flags, int fd, uint32_t offset)

uint32_t _sc_mmap(__unused uint32_t *esp, uint32_t *uesp, int *errno)
{
	process_t *process = process_getCurrentProcess();

	// Get the passed arguments from the stack
	uintptr_t address = *((uintptr_t *)(uesp + 0));
	size_t length     = *((size_t *)(uesp + 1));
	int protection    = *((int *)(uesp + 2));
	int flags         = *((int *)(uesp + 3));

	// Unused stuff
	__unused int filed         = *((int *)(uesp + 4));
	__unused uint32_t offset   = *((int *)(uesp + 5));

	size_t pages = VM_PAGE_COUNT(length);

	process_lock(process);
	process_block(process);

	uintptr_t pmemory = 0x0;
	vm_address_t vmemory = 0x0;
	uint32_t vmflags = mmap_vmflagsForProtectionFlags(protection);

	// Sanity check
	if((flags & MAP_PRIVATE) && (flags & MAP_SHARED))
	{
		*errno = EINVAL;
		return MAP_FAILED;
	}

	if((address && (address % VM_PAGE_SIZE) != 0) || (length % VM_PAGE_SIZE) != 0 || length == 0)
	{
		*errno = EINVAL;
		return MAP_FAILED;
	}

	// Allocate the memory
	pmemory = pm_alloc(pages);
	if(!pmemory)
	{
		*errno = ENOMEM;
		goto mmap_failed;
	}

	if(address)
	{
		vmemory = vm_allocLimit(process->pdirectory, pmemory, pages, (vm_address_t)address, VM_UPPER_LIMIT, vmflags);

		if(vmemory != address && flags & MAP_FIXED)
		{
			*errno = ENOMEM;
			goto mmap_failed;
		}

		if(!vmemory)
			vmemory = vm_alloc(process->pdirectory, pmemory, pages, vmflags);
	}
	else
	{
		// Sorry, no NULL page for you
		if(flags & MAP_FIXED)
		{
			*errno = ENOMEM;
			goto mmap_failed;
		}

		vmemory = vm_alloc(process->pdirectory, pmemory, pages, vmflags);
	}

	if(!vmemory)
	{
		*errno = ENOMEM;
		goto mmap_failed;
	}

	// Map the memory also in kernel address space and initialize it with zeroes
	void *memory = (void *)vm_alloc(vm_getKernelDirectory(), pmemory, pages, VM_FLAGS_KERNEL);
	if(!memory)
	{
		*errno = ENOMEM;
		goto mmap_failed;
	}

	memset(memory, 0, pages * VM_PAGE_SIZE);
	vm_free(vm_getKernelDirectory(), (vm_address_t)memory, pages);

	// Create descriptions for each page
	uintptr_t tpmemory = pmemory;
	vm_address_t tvmemory = vmemory;

	mmap_description_t *description = halloc(NULL, pages * sizeof(mmap_description_t));
	if(!description)
	{
		*errno = ENOMEM;
		goto mmap_failed;
	}

	for(size_t i=0; i<pages; i++)
	{
		description->vaddress = tvmemory;
		description->paddress = tpmemory;
		description->protection = protection;

		atree_insert(process->mappings, description, (void *)tvmemory);

		description ++;
		tvmemory += VM_PAGE_SIZE;
		tpmemory += VM_PAGE_SIZE;
	}

	process_unblock(process);
	process_unlock(process);

	return vmemory;

mmap_failed:
	if(vmemory)
		vm_free(process->pdirectory, vmemory, pages);

	if(pmemory)
		pm_free(pmemory, pages);

	process_unblock(process);
	process_unlock(process);

	return MAP_FAILED;
}

// munmap() signature
// int munmap(void *address, size_t length)

uint32_t _sc_munmap(__unused uint32_t *esp, uint32_t *uesp, int *errno)
{
	uintptr_t address = *((uintptr_t *)(uesp + 0));
	size_t length     = *((size_t *)(uesp + 1));

	// Sanity checks
	if((address % VM_PAGE_SIZE) != 0)
	{
		*errno = EINVAL;
		return -1;
	}

	if((length % VM_PAGE_SIZE) != 0 || length == 0)
	{
		*errno = EINVAL;
		return -1;
	}

	size_t pages = VM_PAGE_COUNT(length);

	process_t *process = process_getCurrentProcess();
	process_lock(process);
	process_block(process);

	for(size_t i=0; i<pages; i++)
	{
		mmap_description_t *description = atree_find(process->mappings, (void *)(address));
		if(!description)
		{
			address += VM_PAGE_SIZE;
			continue;
		}

		vm_mapPage__noLock(process->pdirectory, 0x0, description->vaddress, 0);
		pm_free(description->paddress, 1);

		atree_remove(process->mappings, (void *)address);
		address += VM_PAGE_SIZE;
	}

	process_unblock(process);
	process_unlock(process);

	return 0;
}

// mprotect() signature
// int mprotect(const void *addr, size_t len, int prot)

uint32_t _sc_mprotect(__unused uint32_t *esp, uint32_t *uesp, int *errno)
{
	uintptr_t address = *((uintptr_t *)(uesp + 0));
	size_t length     = *((size_t *)(uesp + 1));
	int protection    = *((int *)(uesp + 2));

	// Sanity checks
	if((address % VM_PAGE_SIZE) != 0)
	{
		*errno = EINVAL;
		return -1;
	}

	if((length % VM_PAGE_SIZE) != 0 || length == 0)
	{
		*errno = EINVAL;
		return -1;
	}

	size_t pages = VM_PAGE_COUNT(length);
	uint32_t vmflags = mmap_vmflagsForProtectionFlags(protection);

	process_t *process = process_getCurrentProcess();
	process_lock(process);
	process_block(process);

	for(size_t i=0; i<pages; i++)
	{
		mmap_description_t *description = atree_find(process->mappings, (void *)(address));
		if(!description)
		{
			address += VM_PAGE_SIZE;
			continue;
		}

		if(description->protection != protection)
		{
			vm_mapPage__noLock(process->pdirectory, description->paddress, description->vaddress, vmflags);
			description->protection = protection;
		}
	}

	process_unblock(process);
	process_unlock(process);

	return 0;
}



void _sc_mmapInit()
{
	sc_setSyscallHandler(SYS_MMAP, _sc_mmap);
	sc_setSyscallHandler(SYS_MUNMAP, _sc_munmap);
	sc_setSyscallHandler(SYS_MPROTECT, _sc_mprotect);
}
