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

#include <config.h>
#include <interrupts/trampoline.h>
#include <system/syslog.h>
#include <system/assert.h>
#include <system/panic.h>
#include <system/kernel.h>
#include <system/lock.h>
#include <libc/string.h>
#include <libc/math.h>
#include "vmemory.h"
#include "pmemory.h"
#include "memory.h"

extern uintptr_t kernelBegin; // Marks the beginning of the kernel (set by the linker)
extern uintptr_t kernelEnd;	// Marks the end of the kernel (also set by the linker)

static vm_page_directory_t __vm_kernelDirectory;
static bool __vm_usePhysicalKernelPages;

static spinlock_t __vm_spinlock = SPINLOCK_INIT;

vm_address_t __vm_alloc_noLock(vm_page_directory_t context, uintptr_t pmemory, size_t pages, uint32_t flags);

#ifndef CONF_VM_NOINLINE
static inline bool __vm_mapPage(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags);
#else
bool __vm_mapPage(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags) __attribute__((noinline));
#endif


// MARK: Inlined functions
// REMARK: These functions assume that pdirectory is mapped!
#ifndef CONF_VM_NOINLINE
static inline vm_address_t __vm_findFreePagesWithLimit(vm_page_directory_t pdirectory, size_t pages, vm_address_t limit)
#else
vm_address_t __vm_findFreePagesWithLimit(vm_page_directory_t pdirectory, size_t pages, vm_address_t limit) __attribute__((noinline));
vm_address_t __vm_findFreePagesWithLimit(vm_page_directory_t pdirectory, size_t pages, vm_address_t limit)
#endif
{
    size_t freePages = 0;
    uint32_t directoryIndex = 0, tableIndex = 0;

    uint32_t index = limit / VM_PAGE_SIZE;
    uint32_t startDirectoryIndex = index / VM_DIRECTORY_LENGTH;
    uint32_t startTableIndex = index % VM_PAGETABLE_LENGTH;

    for(uint32_t i=startDirectoryIndex; i<VM_DIRECTORY_LENGTH; i++)
    {
        if(freePages == 0)
            directoryIndex = i;

        if((pdirectory[i] & VM_PAGETABLEFLAG_PRESENT) || (__vm_kernelDirectory[i] & VM_PAGETABLEFLAG_PRESENT))
        {
            uintptr_t ptable = (uintptr_t)(pdirectory[i] & ~0xFFF);

            vm_page_table_t table1 = (vm_page_table_t)__vm_alloc_noLock(__vm_kernelDirectory, ptable, 1, VM_FLAGS_KERNEL);
            vm_page_table_t table2 = (uint32_t *)(VM_KERNEL_PAGE_TABLES + (i << VM_SHIFT));

            for(uint32_t j=(i == startDirectoryIndex ? startTableIndex : 0); j<VM_PAGETABLE_LENGTH; j++)
            {
                if(!(table1[j] & VM_PAGETABLEFLAG_PRESENT) && !(table2[j] & VM_PAGETABLEFLAG_PRESENT))
                {
                    if(freePages == 0)
                        tableIndex = j;

                    freePages ++;

                    if(freePages >= pages)
                        break;
                }
                else
                {
                    freePages = 0;
                }
            }

            __vm_mapPage(__vm_kernelDirectory, 0x0, (vm_address_t)table1, 0);
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
        syslog(LOG_DEBUG, "Could not find enough free pages. Requested %i pages starting at %p\n", pages, limit);
        return 0x0;
    }

    return (vm_address_t)((directoryIndex << 22) + (tableIndex << VM_SHIFT));
}

#ifndef CONF_VM_NOINLINE
static inline vm_address_t __vm_findFreePages(vm_page_directory_t pdirectory, size_t pages, vm_address_t limit)
#else
vm_address_t __vm_findFreePages(vm_page_directory_t pdirectory, size_t pages, vm_address_t limit) __attribute__((noinline));
vm_address_t __vm_findFreePages(vm_page_directory_t pdirectory, size_t pages, vm_address_t limit)
#endif
{
	size_t freePages = 0;
    uint32_t directoryIndex = 0, tableIndex = 0;

    uint32_t index = limit / VM_PAGE_SIZE;
    uint32_t startDirectoryIndex = index / VM_DIRECTORY_LENGTH;
    uint32_t startTableIndex = index % VM_PAGETABLE_LENGTH;

    if(startDirectoryIndex == 0 && startTableIndex == 0)
        startTableIndex = 1;

    for(uint32_t i=startDirectoryIndex; i<VM_DIRECTORY_LENGTH; i++)
    {
        if(freePages == 0)
            directoryIndex = i;

        if(pdirectory[i] & VM_PAGETABLEFLAG_PRESENT)
        {
        	vm_page_table_t ptable = (vm_page_table_t)(pdirectory[i] & ~0xFFF);
            vm_page_table_t table = (vm_page_table_t)__vm_alloc_noLock(__vm_kernelDirectory, (uintptr_t)ptable, 1, VM_FLAGS_KERNEL);

        	for(uint32_t j=(i == startDirectoryIndex ? startTableIndex : 0); j<VM_PAGETABLE_LENGTH; j++)
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

            __vm_mapPage(__vm_kernelDirectory, 0x0, (vm_address_t)table, 0);
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
    	syslog(LOG_DEBUG, "Could not find enough free pages. Requested %i pages.\n", pages);
        return 0x0;
    }

    return (vm_address_t)((directoryIndex << 22) + (tableIndex << VM_SHIFT));
}

#ifndef CONF_VM_NOINLINE
static inline vm_address_t __vm_findFreeKernelPages(size_t pages, vm_address_t limit)
#else
vm_address_t __vm_findFreeKernelPages(size_t pages, vm_address_t limit) __attribute__((noinline));
vm_address_t __vm_findFreeKernelPages(size_t pages, vm_address_t limit)
#endif
{
	size_t freePages = 0;
    uint32_t directoryIndex = 0, tableIndex = 0;
	vm_page_directory_t pdirectory = __vm_kernelDirectory;

    uint32_t index = limit / VM_PAGE_SIZE;
    uint32_t startDirectoryIndex = index / VM_DIRECTORY_LENGTH;
    uint32_t startTableIndex = index % VM_PAGETABLE_LENGTH;

    if(startDirectoryIndex == 0 && startTableIndex == 0)
        startTableIndex = 1;
	
	for(uint32_t i=startDirectoryIndex; i<VM_DIRECTORY_LENGTH; i++)
	{
		if(freePages == 0)
			directoryIndex = i;
		
		if(pdirectory[i] & VM_PAGETABLEFLAG_PRESENT)
		{
			vm_page_table_t table = (uint32_t *)(VM_KERNEL_PAGE_TABLES + (i << VM_SHIFT));
			
        	for(uint32_t j=(i == startDirectoryIndex ? startTableIndex : 0); j<VM_PAGETABLE_LENGTH; j++)
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
    	panic("Could not find enough free kernel pages. Requested %i pages.\n", pages);
        return 0x0;
    }
	
	return (vm_address_t)((directoryIndex << 22) + (tableIndex << VM_SHIFT));
}

#ifndef CONF_VM_NOINLINE
static inline bool __vm_mapPage(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags)
#else
bool __vm_mapPage(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags) __attribute__((noinline));
bool __vm_mapPage(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags)
#endif
{
	if((((uint32_t)vaddress | (uint32_t)paddress) & 0xFFF))
    {
        panic("__vm_mapPage(), %p or %p (virt, phys) isn't 4k aligned!", vaddress, paddress);
        return false;
    }

    if(vaddress == 0x0 || (paddress == 0x0 && flags != 0))
    {
        panic("Trying to map phys %p to virt %p! (%s)", paddress, vaddress, (pdirectory == __vm_kernelDirectory) ? "kernel directory" : "other directory");
    }

    uint32_t index = vaddress / VM_PAGE_SIZE;
    vm_page_table_t pageTable;
    vm_address_t kernelPage;

    if(!(pdirectory[index / VM_DIRECTORY_LENGTH] & VM_PAGETABLEFLAG_PRESENT))
    {
        pageTable = (vm_page_table_t)pm_alloc(1);
        pdirectory[index / VM_DIRECTORY_LENGTH] = ((uint32_t)pageTable) | VM_FLAGS_USERLAND;

        if(pdirectory == __vm_kernelDirectory)
        {
			if(!__vm_usePhysicalKernelPages)
            	pageTable = (vm_page_table_t)(VM_KERNEL_PAGE_TABLES + ((sizeof(uint32_t) * index) & ~0xFFF));
        }
		else
		{
            // Map the page temporary into memory to avoid bad access
            kernelPage = __vm_findFreeKernelPages(1, 0x0);
            __vm_mapPage(__vm_kernelDirectory, (vm_address_t)pageTable, kernelPage, VM_FLAGS_KERNEL);
            
            pageTable = (vm_page_table_t)kernelPage;
		}
        
        memset(pageTable, 0, VM_PAGETABLE_LENGTH);
    }
    else
    {
        if(pdirectory == __vm_kernelDirectory)
        {
            if(__vm_usePhysicalKernelPages)
            {
                pageTable = (vm_page_table_t)(pdirectory[index / VM_DIRECTORY_LENGTH] & ~0xFFF);
            }
            else
            {
                pageTable = (vm_page_table_t)(VM_KERNEL_PAGE_TABLES + ((sizeof(uint32_t) * index) & ~0xFFF));
            }
        }
        else
        {
            kernelPage = __vm_findFreeKernelPages(1, 0x0);
            pageTable  = (vm_page_table_t)(pdirectory[index / VM_DIRECTORY_LENGTH] & ~0xFFF);

            __vm_mapPage(__vm_kernelDirectory, (vm_address_t)pageTable, kernelPage, VM_FLAGS_KERNEL);
            pageTable = (vm_page_table_t)kernelPage;
        }
    }

    pageTable[index % VM_PAGETABLE_LENGTH] = ((uint32_t)paddress) | flags;

    if(pdirectory != __vm_kernelDirectory)
        __vm_mapPage(__vm_kernelDirectory, 0, (vm_address_t)kernelPage, 0); // Unmap the temporary page table from the kernel
    
    invlpg((uintptr_t)vaddress);
    return true;
}

#ifndef CONF_VM_NOINLINE
static inline bool __vm_mapPageRange(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t vaddress, size_t pages, uint32_t flags)
#else
bool __vm_mapPageRange(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t vaddress, size_t pages, uint32_t flags) __attribute__((noinline));
bool __vm_mapPageRange(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t vaddress, size_t pages, uint32_t flags)
#endif
{
	for(size_t page=0; page<pages; page++)
	{
		bool result = __vm_mapPage(pdirectory, paddress, vaddress, flags);
		if(!result)
			return false;

		paddress += VM_PAGE_SIZE;
		vaddress += VM_PAGE_SIZE;
	}
	
	return true;
}

#ifndef CONF_VM_NOINLINE
static inline uint32_t __vm_getPagetableEntry(vm_page_directory_t pdirectory, vm_address_t vaddress)
#else
uint32_t __vm_getPagetableEntry(vm_page_directory_t pdirectory, vm_address_t vaddress) __attribute__((noinline));
uint32_t __vm_getPagetableEntry(vm_page_directory_t pdirectory, vm_address_t vaddress)
#endif
{
    vm_page_table_t pageTable;
    uintptr_t physPageTable;
    uint32_t result = 0x0;

    uint32_t index = vaddress / VM_PAGE_SIZE;

    if(!(pdirectory[index / VM_PAGETABLE_LENGTH] & VM_PAGETABLEFLAG_PRESENT))
        return result;

    physPageTable = (uintptr_t)(pdirectory[index / VM_PAGETABLE_LENGTH] & ~0xFFF);
    pageTable = (vm_page_table_t)__vm_alloc_noLock(__vm_kernelDirectory, physPageTable, 1, VM_FLAGS_KERNEL);

    if(pageTable[index % VM_PAGETABLE_LENGTH] & VM_PAGETABLEFLAG_PRESENT)
    {
        uint32_t entry = pageTable[index % VM_PAGETABLE_LENGTH];
        result = entry;
    }

    __vm_mapPage(__vm_kernelDirectory, 0x0, (vm_address_t)pageTable, 0);
    return result;
}

// MARK: Public functions
// Might lock the virtual memory lock!
vm_page_directory_t vm_getKernelDirectory()
{
    return __vm_kernelDirectory;
}

vm_page_directory_t vm_createDirectory()
{
    uintptr_t physPageDir = pm_alloc(1);
    vm_page_directory_t directory = (vm_page_directory_t)vm_alloc(__vm_kernelDirectory, physPageDir, 1, VM_FLAGS_KERNEL);

    memset((void *)directory, 0, VM_DIRECTORY_LENGTH * sizeof(vm_page_table_t));
    directory[0xFF] = (uint32_t)physPageDir | VM_FLAGS_KERNEL;

    // Map the trampoline area
    __vm_mapPageRange(directory, IR_TRAMPOLINE_PHYSICAL, IR_TRAMPOLINE_BEGIN, IR_TRAMPOLINE_PAGES, VM_FLAGS_KERNEL);

    // Unmap the directory
    vm_free(__vm_kernelDirectory, (vm_address_t)directory, 1);
    return (vm_page_directory_t)physPageDir;
}

uintptr_t vm_resolveVirtualAddress(vm_page_directory_t pdirectory, vm_address_t vaddress)
{	
    bool isKernelDirectory = true;

    if(pdirectory != __vm_kernelDirectory)
    {
        pdirectory = (vm_page_directory_t)vm_alloc(__vm_kernelDirectory, (uintptr_t)pdirectory, 1, VM_FLAGS_KERNEL);
        isKernelDirectory = false;
    }

    spinlock_lock(&__vm_spinlock);
    uint32_t entry = __vm_getPagetableEntry(pdirectory, vaddress);

    if(!(entry & VM_PAGETABLEFLAG_PRESENT))
    {
        if(!isKernelDirectory)
            __vm_mapPage(__vm_kernelDirectory, 0x0, (vm_address_t)pdirectory, 0);

        spinlock_unlock(&__vm_spinlock);
        return 0x0;
    }


    if(!isKernelDirectory)
        __vm_mapPage(__vm_kernelDirectory, 0x0, (vm_address_t)pdirectory, 0);

    spinlock_unlock(&__vm_spinlock);
    return (uintptr_t)((entry & ~0xFFF) | ((uint32_t)vaddress & 0xFFF));
}



bool vm_mapPage(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags)
{
    bool isKernelDirectory = true;
    bool result;

    if(pdirectory != __vm_kernelDirectory)
    {
        pdirectory = (vm_page_directory_t)vm_alloc(__vm_kernelDirectory, (uintptr_t)pdirectory, 1, VM_FLAGS_KERNEL);
        isKernelDirectory = false;
    }

    spinlock_lock(&__vm_spinlock);
	result = __vm_mapPage(pdirectory, paddress, vaddress, flags);

    if(!isKernelDirectory)
        __vm_mapPage(__vm_kernelDirectory, 0x0, (vm_address_t)pdirectory, 0);

    spinlock_unlock(&__vm_spinlock);
	return result;
}

bool vm_mapPageRange(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t vaddress, size_t pages, uint32_t flags)
{
    bool isKernelDirectory = true;
    bool result;

    if(pdirectory != __vm_kernelDirectory)
    {
        pdirectory = (vm_page_directory_t)vm_alloc(__vm_kernelDirectory, (uintptr_t)pdirectory, 1, VM_FLAGS_KERNEL);
        isKernelDirectory = false;
    }

    spinlock_lock(&__vm_spinlock);
	result = __vm_mapPageRange(pdirectory, paddress, vaddress, pages, flags);

    if(!isKernelDirectory)
        __vm_mapPage(__vm_kernelDirectory, 0x0, (vm_address_t)pdirectory, 0);

    spinlock_unlock(&__vm_spinlock);
	return result;
}


vm_address_t vm_allocTwoSidedLimit(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t limit, size_t pages, uint32_t flags)
{
    if(pdirectory == __vm_kernelDirectory)
    {
        return vm_allocLimit(pdirectory, paddress, limit, pages, flags);
    }


    pdirectory = (vm_page_directory_t)vm_alloc(__vm_kernelDirectory, (uintptr_t)pdirectory, 1, VM_FLAGS_KERNEL);
    spinlock_lock(&__vm_spinlock);

    vm_address_t vaddress = __vm_findFreePagesWithLimit(pdirectory, pages, limit);

    __vm_mapPageRange(__vm_kernelDirectory, paddress, vaddress, pages, flags);
    __vm_mapPageRange(pdirectory, paddress, vaddress, pages, flags);

    __vm_mapPage(__vm_kernelDirectory, 0x0, (vm_address_t)pdirectory, 0);

    spinlock_unlock(&__vm_spinlock);
    return vaddress;
}

vm_address_t vm_allocLimit(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t limit, size_t pages, uint32_t flags)
{
    vm_address_t vaddress = 0x0;
    bool isKernelDirectory = true;

    if(pdirectory != __vm_kernelDirectory)
    {
        pdirectory = (vm_page_directory_t)vm_alloc(__vm_kernelDirectory, (uintptr_t)pdirectory, 1, VM_FLAGS_KERNEL);
        isKernelDirectory = false;
    }

    spinlock_lock(&__vm_spinlock);
    vaddress = (isKernelDirectory) ? __vm_findFreeKernelPages(pages, limit) : __vm_findFreePages(pdirectory, pages, limit);

    if(vaddress)
        __vm_mapPageRange(pdirectory, (vm_address_t)paddress, vaddress, pages, flags);

    if(!isKernelDirectory)
        __vm_mapPage(__vm_kernelDirectory, 0x0, (vm_address_t)pdirectory, 0);

    spinlock_unlock(&__vm_spinlock);
    return vaddress;
}

vm_address_t vm_alloc(vm_page_directory_t pdirectory, uintptr_t paddress, size_t pages, uint32_t flags)
{
	vm_address_t vaddress = 0x0;
    bool isKernelDirectory = true;

    if(pdirectory != __vm_kernelDirectory)
    {
        pdirectory = (vm_page_directory_t)vm_alloc(__vm_kernelDirectory, (uintptr_t)pdirectory, 1, VM_FLAGS_KERNEL);
        isKernelDirectory = false;
    }

    spinlock_lock(&__vm_spinlock);
	vaddress = (isKernelDirectory) ? __vm_findFreeKernelPages(pages, 0x0) : __vm_findFreePages(pdirectory, pages, 0x0);

	if(vaddress)
		__vm_mapPageRange(pdirectory, (vm_address_t)paddress, vaddress, pages, flags);

    if(!isKernelDirectory)
        __vm_mapPage(__vm_kernelDirectory, 0x0, (vm_address_t)pdirectory, 0);

    spinlock_unlock(&__vm_spinlock);
	return vaddress;
}

vm_address_t __vm_alloc_noLock(vm_page_directory_t pdirectory, uintptr_t paddress, size_t pages, uint32_t flags)
{
    vm_address_t vaddress = 0x0;
    bool isKernelDirectory = true;

    if(pdirectory != __vm_kernelDirectory)
    {
        pdirectory = (vm_page_directory_t)__vm_alloc_noLock(__vm_kernelDirectory, (uintptr_t)pdirectory, 1, VM_FLAGS_KERNEL);
        isKernelDirectory = false;
    }

    vaddress = (isKernelDirectory) ? __vm_findFreeKernelPages(pages, 0x0) : __vm_findFreePages(pdirectory, pages, 0x0);

    if(vaddress)
        __vm_mapPageRange(pdirectory, (vm_address_t)paddress, vaddress, pages, flags);

    if(!isKernelDirectory)
        __vm_mapPage(__vm_kernelDirectory, 0x0, (vm_address_t)pdirectory, 0);

    return vaddress;
}

void vm_free(vm_page_directory_t pdirectory, vm_address_t vaddress, size_t pages)
{
    bool isKernelDirectory = true;

    if(pdirectory != __vm_kernelDirectory)
    {
        pdirectory = (vm_page_directory_t)vm_alloc(__vm_kernelDirectory, (uintptr_t)pdirectory, 1, VM_FLAGS_KERNEL);
        isKernelDirectory = false;
    }

    spinlock_lock(&__vm_spinlock);
	for(size_t page=0; page<pages; page++)
	{
		__vm_mapPage(pdirectory, (vm_address_t)0x0, vaddress, 0);
		vaddress += VM_PAGE_SIZE;
	}

    if(!isKernelDirectory)
        __vm_mapPage(__vm_kernelDirectory, 0x0, (vm_address_t)pdirectory, 0);

    spinlock_unlock(&__vm_spinlock);
}



// MARK: Initialization
void vm_createKernelContext()
{
	// Prepare the directory
    __vm_kernelDirectory = (vm_page_directory_t)VM_KERNEL_DIRECTORY_ADDRESS;
	memset(__vm_kernelDirectory, 0, VM_DIRECTORY_LENGTH);

    __vm_kernelDirectory[0xFF] = (uint32_t)__vm_kernelDirectory | VM_FLAGS_KERNEL;

	// Map the page directory
	__vm_mapPage(__vm_kernelDirectory, (vm_address_t)__vm_kernelDirectory, (vm_address_t)__vm_kernelDirectory, VM_FLAGS_KERNEL);
}

void vm_mapMultibootModule(struct multiboot_module_s *module)
{
    // Map the module data
    uintptr_t start = round4kDown((uintptr_t)module->start);
    uintptr_t end = round4kUp((uintptr_t)module->end);

    size_t size = end - start;
    size_t pages = pageCount(size);

    __vm_mapPageRange(__vm_kernelDirectory, start, start, pages, VM_FLAGS_KERNEL);


    // Map the name of the module
    uintptr_t name = round4kDown((uintptr_t)module->name);

    size = strlen((const char *)name);
    pages = pageCount(size);

    __vm_mapPageRange(__vm_kernelDirectory, name, name, pages, VM_FLAGS_KERNEL);
}

void vm_mapMultiboot(struct multiboot_s *info)
{
    struct multiboot_module_s *modules = (struct multiboot_module_s *)info->mods_addr;

    uintptr_t infostart = round4kDown((uintptr_t)info);
    __vm_mapPageRange(__vm_kernelDirectory, infostart, infostart, 1, VM_FLAGS_KERNEL);

    for(uint32_t i=0; i<info->mods_count; i++)
    {
        struct multiboot_module_s *module = &modules[i];
        vm_mapMultibootModule(module);
    }
}

bool vm_init(void *info)
{
    __vm_usePhysicalKernelPages = true;
	vm_createKernelContext();

	// Map the kernel into memory
	vm_address_t _kernelBegin = (vm_address_t)&kernelBegin;
	vm_address_t _kernelEnd   = (vm_address_t)&kernelEnd;

    __vm_mapPageRange(__vm_kernelDirectory, _kernelBegin, _kernelBegin, pageCount(_kernelEnd - _kernelBegin), VM_FLAGS_KERNEL); // Map the kernel
	__vm_mapPageRange(__vm_kernelDirectory, 0xB8000, 0xB8000, 1, VM_FLAGS_KERNEL); // Map the video memory

    // Map the multiboot info
    vm_mapMultiboot((struct multiboot_s *)info);

    __vm_usePhysicalKernelPages = false;

	// Activate the kernel context and paging.
	uint32_t cr0;
	__asm__ volatile("mov %0, %%cr3" : : "r" ((uint32_t)__vm_kernelDirectory));
    __asm__ volatile("mov %%cr0, %0" : "=r" (cr0));
	__asm__ volatile("mov %0, %%cr0" : : "r" (cr0 | (1 << 31)));
	
	return true;
}
