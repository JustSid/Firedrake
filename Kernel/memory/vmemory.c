//
//  vmemory.c
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
#include "vmemory.h"
#include "pmemory.h"

extern uintptr_t kernelBegin; // Marks the beginning of the kernel (set by the linker)
extern uintptr_t kernelEnd;	// Marks the end of the kernel (also set by the linker)

static vm_context_t __vm_kernelContext_s;
static vm_context_t *__vm_kernelContext;

// MARK: Inlined functions
static inline vm_offset_t __vm_getFreePages(vm_context_t *context, size_t pages)
{
	size_t freePages = 0;
    uint32_t directoryIndex = 0, tableIndex = 0;

    for(uint32_t i=0; i<VM_DIRECTORY_LENGTH; i++)
    {
        if(freePages == 0)
            directoryIndex = i;
        
        if(context->pageDirectory[i] & VM_PAGETABLEFLAG_PRESENT)
        {
        	uint32_t *table = (uint32_t *)(context->pageDirectory[i] & ~0xFFF);

        	for(uint32_t j=(i == 0 ? 1 : 0); j<VM_PAGETABLE_LENGTH; j++)
            {
                if(!(table[j] & VM_PAGETABLEFLAG_PRESENT))
                {
                    if(freePages == 0)
                        tableIndex = j;

                    freePages++;
                    if(freePages >= pages)
                        break;
                }
                else
                {
                    freePages = 0;
                }
            }
        }
        else
        {
        	if(i == 0)
            {
                tableIndex = 1;
                freePages += VM_PAGETABLE_LENGTH - 1;
            }
            else
            {
                if(freePages == 0)
                    tableIndex = 0;

                freePages += VM_PAGETABLE_LENGTH;
            }
        }

        if(freePages >= pages)
            break;
    }

 	if(freePages == 0)
    {
    	syslog(LOG_DEBUG, "Could not find enoug free pages. Request %i pages.", pages);
        return 0x0;
    }

    return (vm_offset_t)((directoryIndex << 22) + (tableIndex << 12));
}

static inline bool __vm_mapPage(vm_context_t *context, vm_offset_t physAddress, vm_offset_t virtAddress, uint32_t flags)
{
	if((((uint32_t)virtAddress | (uint32_t)physAddress) & 0xFFF))
    {
        syslog(LOG_DEBUG, "vm_mapPage(), %p or %p (virt, phys) isn't 4k aligned!", virtAddress, physAddress);
        return false;
    }
	
    uint32_t index          = virtAddress / VM_SIZE;
    uint32_t directoryIndex = index / VM_DIRECTORY_LENGTH;
    uint32_t tableIndex     = index % VM_PAGETABLE_LENGTH;
	
	uint32_t *table = (uint32_t *)(context->pageDirectory[directoryIndex] & ~0xFFF);
	table[tableIndex] = physAddress | flags;

	context->pageDirectory[directoryIndex] = ((uint32_t)table) | flags;

	__asm__ volatile("invlpg %0" : : "m" (*(char *)virtAddress));
    return true;
}

static inline bool __vm_mapPageRange(vm_context_t *context, vm_offset_t physAddress, vm_offset_t virtAddress, size_t pages, uint32_t flags)
{
	for(size_t page=0; page<pages; page++)
	{
		bool result = __vm_mapPage(context, physAddress, virtAddress, flags);
		if(!result)
			return false;

		physAddress += VM_SIZE;
		virtAddress += VM_SIZE;
	}
	
	return true;
}


// MARK: Public functions
vm_context_t *vm_getKernelContext()
{
	return __vm_kernelContext;
}

uintptr_t vm_getPhysicalAddress(vm_context_t *context, vm_offset_t virtAddress)
{	
	// TODO: Locking?
	uint32_t index          = virtAddress / VM_SIZE;
    uint32_t directoryIndex = index / VM_DIRECTORY_LENGTH;

	uint32_t table = context->pageDirectory[directoryIndex];
	
	if(!(table & VM_PAGETABLEFLAG_PRESENT))
		return 0x0;
	
	return (uintptr_t)((table & VM_MASK) | ((uint32_t)virtAddress & 0xFFF));
}

bool vm_mapPage(vm_context_t *context, vm_offset_t physAddress, vm_offset_t virtAddress, uint32_t flags)
{
	spinlock_lock(&context->lock);

	bool result = __vm_mapPage(context, physAddress, virtAddress, flags);

	spinlock_unlock(&context->lock);
	return result;
}

bool vm_mapPageRange(vm_context_t *context, vm_offset_t physAddress, vm_offset_t virtAddress, size_t pages, uint32_t flags)
{
	spinlock_lock(&context->lock);

	bool result = __vm_mapPageRange(context, physAddress, virtAddress, pages, flags);
	
	spinlock_unlock(&context->lock);
	return result;
}

vm_offset_t vm_alloc(vm_context_t *context, uintptr_t pmemory, size_t pages, uint32_t flags)
{
	spinlock_lock(&context->lock);

	vm_offset_t tpages = __vm_getFreePages(context, pages);
	if(tpages)
		__vm_mapPageRange(context, (vm_offset_t)pmemory, tpages, pages, flags);

	spinlock_unlock(&context->lock);
	return tpages;
}

void vm_free(vm_context_t *context, vm_offset_t address, size_t pages)
{
	spinlock_lock(&context->lock);

	for(size_t page=0; page<pages; page++)
	{
		__vm_mapPage(context, (vm_offset_t)0x0, address, 0);
		address += VM_SIZE;
	}
	
	spinlock_unlock(&context->lock);
}


// MARK: Initialization
void vm_createKernelContext()
{
	// Prepare the pointer and spinlock
    __vm_kernelContext = &__vm_kernelContext_s;
	__vm_kernelContext->lock.locked = 0;
	__vm_kernelContext->pageDirectory  = (uint32_t *)pm_alloc(1);
   	__vm_kernelContext->directoryStart = pm_alloc(VM_DIRECTORY_LENGTH);

    // Setup and map the page directory
    for(int i=0; i<VM_DIRECTORY_LENGTH; i++)
	{
		uint32_t *table = (uint32_t *)(__vm_kernelContext->directoryStart + (VM_SIZE * i));
		memset(table, 0, VM_PAGETABLE_LENGTH);

		__vm_kernelContext->pageDirectory[i] = (uint32_t)table;
	}

	// Map the page directory
	__vm_mapPageRange(__vm_kernelContext, (vm_offset_t)	__vm_kernelContext->directoryStart, (vm_offset_t)	__vm_kernelContext->directoryStart, VM_DIRECTORY_LENGTH, VM_FLAGS_KERNEL);
	__vm_mapPage(__vm_kernelContext, (vm_offset_t)__vm_kernelContext->pageDirectory, (vm_offset_t)__vm_kernelContext->pageDirectory, VM_FLAGS_KERNEL);
}

bool vm_init(void *unused)
{
	vm_createKernelContext();

	// Map the kernel into memory
	vm_offset_t _kernelBegin = (vm_offset_t)&kernelBegin;
	vm_offset_t _kernelEnd 	 = (vm_offset_t)&kernelEnd;

	for(vm_offset_t page=_kernelBegin; page<_kernelEnd; page+=VM_SIZE)
		__vm_mapPage(__vm_kernelContext, page, page, VM_FLAGS_KERNEL);

	__vm_mapPageRange(__vm_kernelContext, 0xB8000, 0xB8000, 1, VM_FLAGS_KERNEL); // Map the video memory


	// Activate the kernel context and paging.
	uint32_t cr0;
	__asm__ volatile("mov %0, %%cr3" : : "r" ((uint32_t)__vm_kernelContext->pageDirectory));
    __asm__ volatile("mov %%cr0, %0" : "=r" (cr0));
	__asm__ volatile("mov %0, %%cr0" : : "r" (cr0 | (1 << 31)));

	return true;
}
