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
#include <system/panic.h>
#include <libc/string.h>
#include "vmemory.h"
#include "pmemory.h"
#include "memory.h"

extern uintptr_t kernelBegin; // Marks the beginning of the kernel (set by the linker)
extern uintptr_t kernelEnd;	// Marks the end of the kernel (also set by the linker)

static vm_page_directory_t __vm_kernelDirectory;
static uint32_t __vm_kernelDirectoryMutations;
static bool __vm_usePhysicalKernelPages;

// MARK: Inlined functions
static inline vm_offset_t __vm_findFreePages(vm_page_directory_t pdirectory, size_t pages)
{
	size_t freePages = 0;
    uint32_t directoryIndex = 0, tableIndex = 0;

    for(uint32_t i=0; i<VM_DIRECTORY_LENGTH; i++)
    {
        if(freePages == 0)
            directoryIndex = i;
        
        if(pdirectory[i] & VM_PAGETABLEFLAG_PRESENT)
        {
        	vm_page_table_t table = (uint32_t *)(pdirectory[i] & ~0xFFF);

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
    	syslog(LOG_DEBUG, "Could not find enoug free pages. Requested %i pages.", pages);
        return 0x0;
    }

    return (vm_offset_t)((directoryIndex << 22) + (tableIndex << VM_SHIFT));
}

static inline vm_offset_t __vm_findFreeKernelPages(size_t pages)
{
	size_t freePages = 0;
    uint32_t directoryIndex = 0, tableIndex = 0;
	vm_page_directory_t pdirectory = __vm_kernelDirectory;
	
	for(uint32_t i=0; i<VM_DIRECTORY_LENGTH; i++)
	{
		if(freePages == 0)
			directoryIndex = i;
		
		if(pdirectory[i] & VM_PAGETABLEFLAG_PRESENT)
		{
			vm_page_table_t table = (uint32_t *)(VM_KERNEL_PAGE_TABLES + (i << VM_SHIFT));
			
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
    	panic("Could not find enoug free kernel pages. Requested %i pages.", pages);
        return 0x0;
    }
	
	return (vm_offset_t)((directoryIndex << 22) + (tableIndex << VM_SHIFT));
}


static inline bool __vm_mapPage(vm_page_directory_t pdirectory, vm_offset_t paddress, vm_offset_t vaddress, uint32_t flags)
{
	if((((uint32_t)vaddress | (uint32_t)paddress) & 0xFFF))
    {
        syslog(LOG_DEBUG, "__vm_mapPage(), %p or %p (virt, phys) isn't 4k aligned!", vaddress, paddress);
        return false;
    }

    uint32_t index = vaddress / VM_SIZE;
    vm_page_table_t pageTable;
    vm_offset_t kernelPage;

    if(!(pdirectory[index / VM_PAGETABLE_LENGTH] & VM_PAGETABLEFLAG_PRESENT))
    {
        pageTable = (vm_page_table_t)pm_alloc(1);
        pdirectory[index / VM_PAGETABLE_LENGTH] = ((uint32_t)pageTable) | VM_FLAGS_USERLAND;

        if(pdirectory == __vm_kernelDirectory)
        {
			if(!__vm_usePhysicalKernelPages)
            	pageTable = (vm_page_table_t)(VM_KERNEL_PAGE_TABLES + ((sizeof(uint32_t) * index) & ~0xFFF));
        }
		else
		{
            kernelPage = __vm_findFreeKernelPages(1);
            __vm_mapPage(__vm_kernelDirectory, kernelPage, (vm_offset_t)pageTable, VM_FLAGS_KERNEL);
            
            pageTable = (vm_page_table_t)kernelPage;
		}
        

        memset(pageTable, 0, VM_PAGETABLE_LENGTH);

        if(vaddress < VM_USER_START)
            __vm_kernelDirectoryMutations ++;
    }
    else
    {
        if(pdirectory == __vm_kernelDirectory)
        {
            if(__vm_usePhysicalKernelPages)
            {
                pageTable = (vm_page_table_t)(pdirectory[index / VM_PAGETABLE_LENGTH] & ~0xFFF);
            }
            else
            {
                pageTable = (vm_page_table_t)(VM_KERNEL_PAGE_TABLES + ((sizeof(uint32_t) * index) & ~0xFFF));
            }
        }
        else
        {
            kernelPage = __vm_findFreeKernelPages(1);
            pageTable = (vm_page_table_t)(pdirectory[index / VM_PAGETABLE_LENGTH] & ~0xFFF);

            __vm_mapPage(__vm_kernelDirectory, kernelPage, (vm_offset_t)pageTable, VM_FLAGS_KERNEL);
            pageTable = (vm_page_table_t)kernelPage;
        }
    }


    pageTable[index % VM_PAGETABLE_LENGTH] = ((uint32_t)paddress) | flags;

    if(pdirectory != __vm_kernelDirectory)
        vm_mapPage(__vm_kernelDirectory, kernelPage, (vm_offset_t)pageTable, 0); // Unmap the temporary page table from the kernel
    
	__asm__ volatile("invlpg %0" : : "m" (*(char *)vaddress));
    return true;
}

static inline bool __vm_mapPageRange(vm_page_directory_t pdirectory, vm_offset_t paddress, vm_offset_t vaddress, size_t pages, uint32_t flags)
{
	for(size_t page=0; page<pages; page++)
	{
		bool result = __vm_mapPage(pdirectory, paddress, vaddress, flags);
		if(!result)
			return false;

		paddress += VM_SIZE;
		vaddress += VM_SIZE;
	}
	
	return true;
}


// MARK: Public functions
vm_page_directory_t vm_getKernelDirectory()
{
	return __vm_kernelDirectory;
}

vm_page_directory_t vm_getCurrentDirectory()
{
	uint32_t address;
	__asm__ volatile("mov %%cr3, %0" : "=r" (address)); 
	
    return (vm_page_directory_t)address;
}

vm_page_directory_t vm_createDirectory()
{
	vm_page_directory_t directory = ualloc(1);
	if(directory)
	{
    	memset((void *)directory, 0, VM_SIZE);
    	memcpy((void *)directory, __vm_kernelDirectory, 1024);
	}
	
    return directory;
}

uintptr_t vm_getPhysicalAddress(vm_page_directory_t pdirectory, vm_offset_t vaddress)
{	
    uint32_t index = vaddress / VM_SIZE;

    if(!(pdirectory[index / VM_DIRECTORY_LENGTH] & VM_PAGETABLEFLAG_PRESENT))
        return 0x0;

    vm_page_table_t table = (vm_page_table_t)(pdirectory[index / VM_DIRECTORY_LENGTH] & ~0xFFF);
	return (table[index % VM_PAGETABLE_LENGTH] & ~0xFFF);
}

bool vm_mapPage(vm_page_directory_t pdirectory, vm_offset_t paddress, vm_offset_t vaddress, uint32_t flags)
{
	bool result = __vm_mapPage(pdirectory, paddress, vaddress, flags);
	return result;
}

bool vm_mapPageRange(vm_page_directory_t pdirectory, vm_offset_t paddress, vm_offset_t vaddress, size_t pages, uint32_t flags)
{
	bool result = __vm_mapPageRange(pdirectory, paddress, vaddress, pages, flags);
	return result;
}



vm_offset_t vm_alloc(vm_page_directory_t pdirectory, uintptr_t paddress, size_t pages, uint32_t flags)
{
	vm_offset_t vaddress = 0x0;
	
	vaddress = (pdirectory == __vm_kernelDirectory) ? __vm_findFreeKernelPages(pages) : __vm_findFreePages(pdirectory, pages);
	
	if(vaddress)
		__vm_mapPageRange(pdirectory, (vm_offset_t)paddress, vaddress, pages, flags);

	return vaddress;
}

void vm_free(vm_page_directory_t pdirectory, vm_offset_t vaddress, size_t pages)
{
	for(size_t page=0; page<pages; page++)
	{
		__vm_mapPage(pdirectory, (vm_offset_t)0x0, vaddress, 0);
		vaddress += VM_SIZE;
	}
}


void vm_activateContext(vm_page_directory_t pdirectory)
{
	__asm__ volatile("mov %0, %%cr3" : : "r" ((uint32_t)pdirectory));
}


// MARK: Initialization
void vm_createKernelContext()
{
	// Prepare the directory
    __vm_kernelDirectory = (vm_page_directory_t)pm_allocLimit(0x1000, 1);
	memset(__vm_kernelDirectory, 0, VM_DIRECTORY_LENGTH);
    __vm_kernelDirectory[VM_KERNEL_PAGE_TABLES >> 22] = (uint32_t)__vm_kernelDirectory | VM_FLAGS_KERNEL;

	// Map the page directory
	__vm_mapPage(__vm_kernelDirectory, (vm_offset_t)__vm_kernelDirectory, (vm_offset_t)__vm_kernelDirectory, VM_FLAGS_KERNEL);
}

bool vm_init(void *unused)
{
    __vm_usePhysicalKernelPages = true;
	vm_createKernelContext();

	// Map the kernel into memory
	vm_offset_t _kernelBegin = (vm_offset_t)&kernelBegin;
	vm_offset_t _kernelEnd 	 = (vm_offset_t)&kernelEnd;

    __vm_mapPageRange(__vm_kernelDirectory, _kernelBegin, _kernelBegin, ((uint32_t)_kernelEnd - (uint32_t)_kernelBegin) / VM_SIZE, VM_FLAGS_KERNEL);
	__vm_mapPageRange(__vm_kernelDirectory, 0xB8000, 0xB8000, 1, VM_FLAGS_KERNEL); // Map the video memory

    __vm_usePhysicalKernelPages = false;

	// Activate the kernel context and paging.
	uint32_t cr0;
	__asm__ volatile("mov %0, %%cr3" : : "r" ((uint32_t)__vm_kernelDirectory));
    __asm__ volatile("mov %%cr0, %0" : "=r" (cr0));
	__asm__ volatile("mov %0, %%cr0" : : "r" (cr0 | (1 << 31)));

	// "Move" the kernel
	// TODO: Move the kernel!
	
	return true;
}
