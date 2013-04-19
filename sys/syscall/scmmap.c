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
#include <system/syslog.h>
#include <libc/string.h>

#include "scmmap.h"
#include "syscall.h"

uint32_t mmap_vmflagsForProtectionFlags(int protection)
{
	uint32_t vmflags = VM_PAGETABLEFLAG_PRESENT;

	if(!(protection & PROT_NONE))
		vmflags |= VM_FLAGS_USERLAND;

	if(!(protection & PROT_READ) && (protection & PROT_WRITE))
		vmflags |= VM_PAGETABLEFLAG_WRITEABLE;

	return vmflags;
}

bool mmap_copyMappings(process_t *target, process_t *source)
{
	list_lock(target->mappings);
	list_lock(source->mappings);

	mmap_description_t *srcDescription = list_first(source->mappings);
	while(srcDescription)
	{
		mmap_description_t *dstDescription = list_addBack(target->mappings);

		size_t pages = pageCount(srcDescription->length);
		dstDescription->paddress   = pm_alloc(pages);
		dstDescription->vaddress   = srcDescription->vaddress;
		dstDescription->protection = srcDescription->protection;
		dstDescription->length     = srcDescription->length;

		vm_mapPageRange(target->pdirectory, dstDescription->paddress, dstDescription->vaddress, pages, mmap_vmflagsForProtectionFlags(dstDescription->protection));

		// Copy the mappings content
		void *srcmem = (void *)vm_alloc(vm_getKernelDirectory(), srcDescription->paddress, pages, VM_FLAGS_KERNEL);
		void *dstmem = (void *)vm_alloc(vm_getKernelDirectory(), dstDescription->paddress, pages, VM_FLAGS_KERNEL);

		memcpy(dstmem, srcmem, srcDescription->length);

		vm_free(vm_getKernelDirectory(), (vm_address_t)srcmem, pages);
		vm_free(vm_getKernelDirectory(), (vm_address_t)dstmem, pages);

		srcDescription = srcDescription->next;
	}

	list_unlock(source->mappings);
	list_unlock(target->mappings);

	return true;
}

// Splits a mmap_description_t from address to address + length
void mmap_splitDescription(mmap_description_t *description, vm_address_t address, size_t length, mmap_description_t **prevOut, mmap_description_t **nextOut)
{
	process_t *process = (process_t *)description->process;

	if(prevOut)
		*prevOut = NULL;

	if(nextOut)
		*nextOut = NULL;

	if((length % 4096) != 0)
		length = round4kUp(length);

	if(description->length == length)
		return;

	if(description->vaddress == address)
	{
		mmap_description_t *next = list_addBack(process->mappings);
		next->paddress   = description->paddress + length;
		next->vaddress   = description->vaddress + length;
		next->length     = description->length - length;
		next->protection = description->protection;
		next->next       = description->next;
		next->process    = description->process;

		description->length = description->length - next->length;
		description->next = next;

		if(nextOut)
			*nextOut = next;
	}
	else if(description->vaddress + description->length == address + length)
	{
		mmap_description_t *prev = list_addBack(process->mappings);
		prev->paddress   = description->paddress;
		prev->vaddress   = description->vaddress;
		prev->length     = description->length - length;
		prev->protection = description->protection;
		prev->prev       = description->prev;
		prev->process    = description->process;

		description->vaddress = prev->vaddress + prev->length;
		description->paddress = prev->paddress + prev->length;
		description->length   = description->length - prev->length;
		description->prev     = prev;

		if(prevOut)
			*prevOut = prev;
	}
	else
	{
		mmap_description_t *prev = list_addBack(process->mappings);
		prev->paddress   = description->paddress;
		prev->vaddress   = description->vaddress;
		prev->length     = address - description->vaddress;
		prev->protection = description->protection;
		prev->prev       = description->prev;
		prev->process    = description->process;

		mmap_description_t *next = list_addBack(process->mappings);
		next->paddress   = description->paddress + length;
		next->vaddress   = description->vaddress + length;
		next->length     = (description->vaddress + description->length) - (address + length);
		next->protection = description->protection;
		next->next       = description->next;
		next->process    = description->process;

		description->vaddress = prev->vaddress + prev->length;
		description->paddress = prev->paddress + prev->length;
		description->length   = description->length - (prev->length + next->length);
		description->prev     = prev;
		description->next     = next;

		if(nextOut)
			*nextOut = next;

		if(prevOut)
			*prevOut = prev;
	}
}

// Joins the two descriptions
// 
void mmap_joinDescription(mmap_description_t *description, mmap_description_t *joining)
{
	assert((joining == description->next || joining == description->prev));

	process_t *process = (process_t *)description->process;

	if(description->next == joining)
	{
		description->next = joining->next;
		description->length += joining->length;
		
		list_remove(process->mappings, joining);
	}
	else
	{
		description->prev = joining->prev;
		description->length += joining->length;
		description->vaddress = joining->vaddress;
		description->paddress = joining->paddress;

		list_remove(process->mappings, joining);
	}
}

bool mmap_tryJoinFragments(mmap_description_t *description, vm_address_t address, size_t length)
{
	// Check if the mapping is just fragmented
	mmap_description_t *last = NULL;
	mmap_description_t *current = description->next;

	size_t lengthLeft = (address + length) - (description->vaddress + description->length);
	while(current)
	{
		if(lengthLeft <= current->length)
		{
			size_t overflow = current->length - lengthLeft;
			if(overflow > 0)
				mmap_splitDescription(current, current->vaddress, overflow, NULL, NULL);

			last = current;
			break;
		}

		lengthLeft -= current->length;
		current = current->next;
	}

	if(last)
	{
		// Join the fragments together
		current = description->next;
		while(1)
		{
			mmap_joinDescription(description, current);
			if(current == last)
				break;

			current = description->next;
		}

		return true;
	}

	return false;
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

	list_lock(process->mappings);
	mmap_description_t *description = list_addBack(process->mappings);

	if(!description)
	{
		list_unlock(process->mappings);

		*errno = ENOMEM;
		return MAP_FAILED;
	}

	description->process = process;

	if((flags & MAP_PRIVATE) && (flags & MAP_SHARED))
		goto mmapFailed;

	if((flags & MAP_PRIVATE) && (flags & MAP_ANONYMOUS))
	{
		// Check if address and length are on an 4k aligned
		if((address % 4096) != 0 || (length % 4096) != 0)
		{
			list_unlock(process->mappings);

			*errno = EINVAL;
			return MAP_FAILED;
		}

		// Get the right virtual memory flags for the requested protection bits
		uint32_t vmflags = mmap_vmflagsForProtectionFlags(protection);

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

		// Update the description
		description->vaddress   = vmemory;
		description->paddress   = pmemory;
		description->length     = length;
		description->protection = protection;

		list_unlock(process->mappings);
		return vmemory;
	}


mmapFailed:

	list_remove(process->mappings, description);
	list_unlock(process->mappings);

	return MAP_FAILED;
}

// munmap() signature
// int munmap(void *address, size_t length)
uint32_t _sc_munmap(__unused uint32_t *esp, uint32_t *uesp, int *errno)
{
	process_t *process = process_getCurrentProcess();

	// fetch the arguments from the stack
	uintptr_t address = *((uintptr_t *)(uesp + 0));
	size_t length = *((size_t *)(uesp + 1));

	if((address % 4096) != 0 || length == 0)
	{
		*errno = EINVAL;
		return -1;
	}

	list_lock(process->mappings);

	// Search for the mmap entry that fits the address and length
	mmap_description_t *description = list_first(process->mappings); 
	while(description)
	{
		if(description->vaddress >= address && address <= description->vaddress + description->length)
		{
			if(address + length > description->vaddress + description->length)
			{
				if(!mmap_tryJoinFragments(description, address, length))
				{
					*errno = ENOMEM;
					return -1;
				}
			}

			mmap_splitDescription(description, address, length, NULL, NULL);
			size_t pages = pageCount(description->length);

			vm_free(process->pdirectory, description->vaddress, pages);
			pm_free(description->vaddress, pages);

			list_remove(process->mappings, description);
			list_unlock(process->mappings);
			return 0;
		}

		description = description->next;
	}

	list_unlock(process->mappings);
	
	*errno = EINVAL;
	return -1;
}

// mprotect() signature
// int mprotect(const void *addr, size_t len, int prot)

uint32_t _sc_mprotect(__unused uint32_t *esp, uint32_t *uesp, int *errno)
{
	process_t *process = process_getCurrentProcess();

	// fetch the arguments from the stack
	uintptr_t address = *((uintptr_t *)(uesp + 0));
	size_t length     = *((size_t *)(uesp + 1));
	int  protection   = *((int *)(uesp + 2));

	if((address % 4096) != 0 || length == 0)
	{
		*errno = EINVAL;
		return -1;
	}


	list_lock(process->mappings);

	mmap_description_t *description = list_first(process->mappings); 
	while(description)
	{
		if(description->vaddress >= address && address <= description->vaddress + description->length)
		{
			if(address + length > description->vaddress + description->length)
			{
				if(!mmap_tryJoinFragments(description, address, length))
				{
					*errno = ENOMEM;
					return -1;
				}
			}

			mmap_splitDescription(description, address, length, NULL, NULL);

			uint32_t vmflags = mmap_vmflagsForProtectionFlags(protection);
			size_t pages = pageCount(description->length);

			vm_mapPageRange(process->pdirectory, description->paddress, description->vaddress, pages, vmflags);
			description->protection = protection;

			list_unlock(process->mappings);
			return 0;
		}

		description = description->next;
	}

	list_unlock(process->mappings);

	*errno = EINVAL;
	return -1;
}



void _sc_mmapInit()
{
	sc_setSyscallHandler(SYS_MMAP, _sc_mmap);
	sc_setSyscallHandler(SYS_MUNMAP, _sc_munmap);
	sc_setSyscallHandler(SYS_MPROTECT, _sc_mprotect);
}
