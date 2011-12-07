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
        
        if(context->directory[i] & VM_PAGETABLEFLAG_PRESENT)
        {
        	uint32_t *table = (uint32_t *)(context->directory[i] & ~0xFFF);

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

static inline bool __vm_mapPage(vm_context_t *context, vm_offset_t paddress, vm_offset_t vaddress, uint32_t flags)
{
	if((((uint32_t)vaddress | (uint32_t)paddress) & 0xFFF))
    {
        syslog(LOG_DEBUG, "__vm_mapPage(), %p or %p (virt, phys) isn't 4k aligned!", vaddress, paddress);
        return false;
    }

    uint32_t index = vaddress / VM_SIZE;

    vm_page_directory_t directory = context->directory;
    vm_page_table_t table = (vm_page_table_t)(directory[index / VM_DIRECTORY_LENGTH] & ~0xFFF);

    table[index % VM_PAGETABLE_LENGTH] = ((uint32_t)paddress) | flags;

	__asm__ volatile("invlpg %0" : : "m" (*(char *)vaddress));
    return true;
}

static inline bool __vm_mapPageRange(vm_context_t *context, vm_offset_t paddress, vm_offset_t vaddress, size_t pages, uint32_t flags)
{
	for(size_t page=0; page<pages; page++)
	{
		bool result = __vm_mapPage(context, paddress, vaddress, flags);
		if(!result)
			return false;

		paddress += VM_SIZE;
		vaddress += VM_SIZE;
	}
	
	return true;
}


// MARK: Public functions
vm_context_t *vm_getKernelContext()
{
	return __vm_kernelContext;
}

uintptr_t vm_getPhysicalAddress(vm_context_t *context, vm_offset_t vaddress)
{	
	// TODO: Locking?
    uint32_t index = vaddress / VM_SIZE;

    vm_page_directory_t directory = context->directory;
    vm_page_table_t table = (vm_page_table_t)(directory[index / VM_DIRECTORY_LENGTH] & ~0xFFF);

    uintptr_t result = table[index % VM_PAGETABLE_LENGTH];

	return (uintptr_t)(result & ~0xFFF);
}

bool vm_mapPage(vm_context_t *context, vm_offset_t paddress, vm_offset_t vaddress, uint32_t flags)
{
	spinlock_lock(&context->lock);

	bool result = __vm_mapPage(context, paddress, vaddress, flags);

	spinlock_unlock(&context->lock);
	return result;
}

bool vm_mapPageRange(vm_context_t *context, vm_offset_t paddress, vm_offset_t vaddress, size_t pages, uint32_t flags)
{
	spinlock_lock(&context->lock);

	bool result = __vm_mapPageRange(context, paddress, vaddress, pages, flags);
	
	spinlock_unlock(&context->lock);
	return result;
}



vm_offset_t vm_alloc(vm_context_t *context, uintptr_t paddress, size_t pages, uint32_t flags)
{
	spinlock_lock(&context->lock);

	vm_offset_t vaddress = __vm_getFreePages(context, pages);
	if(vaddress)
		__vm_mapPageRange(context, (vm_offset_t)paddress, vaddress, pages, flags);

	spinlock_unlock(&context->lock);
	return vaddress;
}

void vm_free(vm_context_t *context, vm_offset_t vaddress, size_t pages)
{
	spinlock_lock(&context->lock);

	for(size_t page=0; page<pages; page++)
	{
		__vm_mapPage(context, (vm_offset_t)0x0, vaddress, 0);
		vaddress += VM_SIZE;
	}
	
	spinlock_unlock(&context->lock);
}


void vm_activateContext(vm_context_t *context)
{
	__asm__ volatile("mov %0, %%cr3" : : "r" ((uint32_t)context->directory));
}


// MARK: Initialization
void vm_createKernelContext()
{
	// Prepare the pointer and spinlock
    __vm_kernelContext = &__vm_kernelContext_s;
	__vm_kernelContext->lock.locked = 0;
	__vm_kernelContext->directory  = (vm_page_directory_t)pm_alloc(1);
   	__vm_kernelContext->directoryStart = pm_alloc(VM_DIRECTORY_LENGTH);

    // Setup and map the page directory
    for(int i=0; i<VM_DIRECTORY_LENGTH; i++)
	{
		vm_page_table_t table = (vm_page_table_t)(__vm_kernelContext->directoryStart + (VM_SIZE * i));
		memset(table, 0, VM_PAGETABLE_LENGTH);

		__vm_kernelContext->directory[i] = ((uint32_t)table) | VM_FLAGS_KERNEL;
	}

	// Map the page directory
	__vm_mapPageRange(__vm_kernelContext, (vm_offset_t)	__vm_kernelContext->directoryStart, (vm_offset_t)__vm_kernelContext->directoryStart, VM_DIRECTORY_LENGTH, VM_FLAGS_KERNEL);
	__vm_mapPage(__vm_kernelContext, (vm_offset_t)__vm_kernelContext->directory, (vm_offset_t)__vm_kernelContext->directory, VM_FLAGS_KERNEL);
}

bool vm_init(void *unused)
{
	vm_createKernelContext();

	// Map the kernel into memory
	vm_offset_t _kernelBegin = (vm_offset_t)&kernelBegin;
	vm_offset_t _kernelEnd 	 = (vm_offset_t)&kernelEnd;

	for(vm_offset_t page=_kernelBegin; page<_kernelEnd;)
	{
		__vm_mapPage(__vm_kernelContext, page, page, VM_FLAGS_KERNEL);
		page += VM_SIZE;
	}

	__vm_mapPageRange(__vm_kernelContext, 0xB8000, 0xB8000, 1, VM_FLAGS_KERNEL); // Map the video memory


	// Activate the kernel context and paging.
	uint32_t cr0;
	__asm__ volatile("mov %0, %%cr3" : : "r" ((uint32_t)__vm_kernelContext->directory));
    __asm__ volatile("mov %%cr0, %0" : "=r" (cr0));
	__asm__ volatile("mov %0, %%cr0" : : "r" (cr0 | (1 << 31)));

	// "Move" the kernel

	return true;
}
