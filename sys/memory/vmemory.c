//
//  vmemory.c
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

#include <interrupts/trampoline.h>
#include <system/syslog.h>
#include <libc/assert.h>
#include <system/panic.h>
#include <system/kernel.h>
#include <system/lock.h>
#include <libc/string.h>
#include <libc/math.h>
#include "vmemory.h"
#include "pmemory.h"
#include "memory.h"
#include "dma.h"

extern uintptr_t kernelBegin; // Marks the beginning of the kernel (set by the linker)
extern uintptr_t kernelEnd;	// Marks the end of the kernel (also set by the linker)

static vm_page_directory_t __vm_kernelDirectory;
static bool __vm_usePhysicalKernelPages;

static spinlock_t __vm_spinlock = SPINLOCK_INIT;

#if CONF_RELEASE
#define __vm_assert(e, ...) (void)0
#else
#define __vm_assert(e, ...) if(__builtin_expect(!(e), 0)) \
	{ \
		warn("%s:%i: Assertion \'%s\' failed.", __func__, __LINE__, #e); \
		warn(__VA_ARGS__); \
		panic(""); \
	}

#endif

#if CONF_VM_NOINLINE
__noinline vm_address_t __vm_findFreePages__user(vm_page_directory_t directory, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);
__noinline vm_address_t __vm_findFreePages__kernel(size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);
__noinline vm_address_t __vm_findFreePagesTwoSided(vm_page_directory_t directory, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);
__noinline vm_address_t __vm_findFreePages(vm_page_directory_t directory, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);

__noinline void __vm_mapPage__noLock(vm_page_directory_t directory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags);
__noinline void __vm_mapPageRange__noLock(vm_page_directory_t directory, uintptr_t paddress, vm_address_t vaddress, size_t pages, uint32_t flags);
#else
__inline vm_address_t __vm_findFreePages__user(vm_page_directory_t directory, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);
__inline vm_address_t __vm_findFreePages__kernel(size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);
__inline vm_address_t __vm_findFreePagesTwoSided(vm_page_directory_t directory, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);
__inline vm_address_t __vm_findFreePages(vm_page_directory_t directory, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);

__inline void __vm_mapPage__noLock(vm_page_directory_t directory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags);
__inline void __vm_mapPageRange__noLock(vm_page_directory_t directory, uintptr_t paddress, vm_address_t vaddress, size_t pages, uint32_t flags);
#endif

vm_address_t __vm_alloc_noLock(vm_page_directory_t pdirectory, uintptr_t paddress, size_t pages, uint32_t flags);


vm_address_t __vm_findFreePages__user(vm_page_directory_t directory, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit)
{
	__vm_assert(pages > 0 && lowerLimit >= VM_LOWER_LIMIT && upperLimit <= VM_UPPER_LIMIT, "%i %x %x", pages, lowerLimit, upperLimit);
	__vm_assert((lowerLimit % VM_PAGE_SIZE) == 0, "%x", lowerLimit);
	__vm_assert((upperLimit % VM_PAGE_SIZE) == 0, "%x", upperLimit);

	vm_address_t regionStart = 0x0;

	uint32_t pageTableIndex = (lowerLimit >> VM_DIRECTORY_SHIFT);
	uint32_t pageIndex = (lowerLimit >> VM_PAGE_SHIFT) % VM_PAGETABLE_LENGTH;

	size_t foundPages = 0;
	while(foundPages < pages && (pageTableIndex << VM_DIRECTORY_SHIFT) < upperLimit)
	{
		if(pageTableIndex >= VM_DIRECTORY_LENGTH)
		{
			foundPages = 0;
			break;
		}

		if(directory[pageTableIndex] & VM_PAGETABLEFLAG_PRESENT)
		{
			vm_page_table_t ptable = (vm_page_table_t)(directory[pageTableIndex] & ~0xFFF);
			vm_page_table_t table = (vm_page_table_t)__vm_alloc_noLock(__vm_kernelDirectory, (uintptr_t)ptable, 1, VM_FLAGS_KERNEL);
		
			while(pageIndex < VM_PAGETABLE_LENGTH)
			{
				if(!(table[pageIndex] & VM_PAGETABLEFLAG_PRESENT))
				{
					if(foundPages == 0)
						regionStart = (pageTableIndex << VM_DIRECTORY_SHIFT) + (pageIndex << VM_PAGE_SHIFT);

					if((++ foundPages) >= pages)
						break;
				}
				else
				{
					foundPages = 0;
				}

				pageIndex ++;
			}

			__vm_mapPage__noLock(__vm_kernelDirectory, 0x0, (vm_address_t)table, 0);
		}
		else
		{
			if(foundPages == 0)
				regionStart = (pageTableIndex << VM_DIRECTORY_SHIFT) + (pageIndex << VM_PAGE_SHIFT);

			foundPages += VM_PAGETABLE_LENGTH;
		}

		pageIndex = 0;
		pageTableIndex ++;
	}

	if(foundPages >= pages && regionStart + (pages * VM_PAGE_SIZE) <= upperLimit)
		return regionStart;

	warn("Couldn't find %u pages! Limit: {%x, %x}\n", pages, lowerLimit, upperLimit);
	return 0x0;
}

vm_address_t __vm_findFreePages__kernel(size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit)
{
	__vm_assert(pages > 0 && lowerLimit >= VM_LOWER_LIMIT && upperLimit <= VM_UPPER_LIMIT, "%i %x %x", pages, lowerLimit, upperLimit);
	__vm_assert((lowerLimit % VM_PAGE_SIZE) == 0, "%x", lowerLimit);
	__vm_assert((upperLimit % VM_PAGE_SIZE) == 0, "%x", upperLimit);

	vm_address_t regionStart = 0x0;

	uint32_t pageTableIndex = (lowerLimit >> VM_DIRECTORY_SHIFT);
	uint32_t pageIndex = (lowerLimit >> VM_PAGE_SHIFT) % VM_PAGETABLE_LENGTH;

	size_t foundPages = 0;
	while(foundPages < pages && (pageTableIndex << VM_DIRECTORY_SHIFT) < upperLimit)
	{
		if(pageTableIndex >= VM_DIRECTORY_LENGTH)
		{
			foundPages = 0;
			break;
		}

		if(__vm_kernelDirectory[pageTableIndex] & VM_PAGETABLEFLAG_PRESENT)
		{
			vm_page_table_t table = (uint32_t *)(VM_KERNEL_PAGE_TABLES + (pageTableIndex << VM_PAGE_SHIFT));
			
			while(pageIndex < VM_PAGETABLE_LENGTH)
			{
				if(!(table[pageIndex] & VM_PAGETABLEFLAG_PRESENT))
				{
					if(foundPages == 0)
						regionStart = (pageTableIndex << VM_DIRECTORY_SHIFT) + (pageIndex << VM_PAGE_SHIFT);

					if((++ foundPages) >= pages)
						break;
				}
				else
				{
					foundPages = 0;
				}

				pageIndex ++;
			}
		}
		else
		{
			if(foundPages == 0)
				regionStart = (pageTableIndex << VM_DIRECTORY_SHIFT) + (pageIndex << VM_PAGE_SHIFT);

			foundPages += VM_PAGETABLE_LENGTH;
		}

		pageIndex = 0;
		pageTableIndex ++;
	}

	if(foundPages >= pages && regionStart + (pages * VM_PAGE_SIZE) <= upperLimit)
		return regionStart;

	warn("Couldn't find %u pages! Limit: {%x, %x}\n", pages, lowerLimit, upperLimit);
	return 0x0;
}

vm_address_t __vm_findFreePagesTwoSided(vm_page_directory_t directory, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit)
{
	__vm_assert(pages > 0 && lowerLimit >= VM_LOWER_LIMIT && upperLimit <= VM_UPPER_LIMIT, "%i %x %x", pages, lowerLimit, upperLimit);
	__vm_assert((lowerLimit % VM_PAGE_SIZE) == 0, "%x", lowerLimit);
	__vm_assert((upperLimit % VM_PAGE_SIZE) == 0, "%x", upperLimit);

	vm_address_t regionStart = 0x0;

	uint32_t pageTableIndex = (lowerLimit >> VM_DIRECTORY_SHIFT);
	uint32_t pageIndex = (lowerLimit >> VM_PAGE_SHIFT) % VM_PAGETABLE_LENGTH;

	size_t foundPages = 0;
	while(foundPages < pages && (pageTableIndex << VM_DIRECTORY_SHIFT) < upperLimit)
	{
		if(pageTableIndex >= VM_DIRECTORY_LENGTH)
		{
			foundPages = 0;
			break;
		}
		
		if(directory[pageTableIndex] & VM_PAGETABLEFLAG_PRESENT || __vm_kernelDirectory[pageTableIndex] & VM_PAGETABLEFLAG_PRESENT)
		{
			vm_page_table_t utable = 0x0;
			vm_page_table_t ktable = 0x0;

			if(directory[pageTableIndex] & VM_PAGETABLEFLAG_PRESENT)
			{
				vm_page_table_t ptable = (vm_page_table_t)(directory[pageTableIndex] & ~0xFFF);
				utable = (vm_page_table_t)__vm_alloc_noLock(__vm_kernelDirectory, (uintptr_t)ptable, 1, VM_FLAGS_KERNEL);
			}

			if(__vm_kernelDirectory[pageTableIndex] & VM_PAGETABLEFLAG_PRESENT)
			{
				ktable = (uint32_t *)(VM_KERNEL_PAGE_TABLES + (pageTableIndex << VM_PAGE_SHIFT));
			}

			
			while(pageIndex < VM_PAGETABLE_LENGTH)
			{
				bool isFree = true;

				if(utable && utable[pageIndex] & VM_PAGETABLEFLAG_PRESENT)
					isFree = false;

				if(ktable && ktable[pageIndex] & VM_PAGETABLEFLAG_PRESENT)
					isFree = false;

				if(isFree)
				{
					if(foundPages == 0)
						regionStart = (pageTableIndex << VM_DIRECTORY_SHIFT) + (pageIndex << VM_PAGE_SHIFT);

					if((++ foundPages) >= pages)
						break;
				}
				else
				{
					foundPages = 0;
				}

				pageIndex ++;
			}

			if(utable)
				__vm_mapPage__noLock(__vm_kernelDirectory, 0x0, (vm_address_t)utable, 0);
		}
		else
		{
			if(foundPages == 0)
				regionStart = (pageTableIndex << VM_DIRECTORY_SHIFT) + (pageIndex << VM_PAGE_SHIFT);

			foundPages += VM_PAGETABLE_LENGTH;
		}

		pageIndex = 0;
		pageTableIndex ++;
	}

	if(foundPages >= pages && regionStart + (pages * VM_PAGE_SIZE) <= upperLimit)
		return regionStart;

	warn("Couldn't find %u pages! Limit: {%x, %x}\n", pages, lowerLimit, upperLimit);
	return 0x0;
}

vm_address_t __vm_findFreePages(vm_page_directory_t directory, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit)
{
	vm_address_t vaddress = 0x0;
	vaddress = (directory == __vm_kernelDirectory) ? __vm_findFreePages__kernel(pages, lowerLimit, upperLimit) : __vm_findFreePages__user(directory, pages, lowerLimit, upperLimit);
	return vaddress;
}




void __vm_mapPage__noLock(vm_page_directory_t directory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags)
{
	__vm_assert(vaddress > 0x0, "%x", vaddress);
	__vm_assert(paddress > 0 || flags == 0, "%x %x", paddress, flags);
	__vm_assert((vaddress % VM_PAGE_SIZE) == 0, "%x", vaddress);
	__vm_assert((paddress % VM_PAGE_SIZE) == 0, "%x", vaddress);

	uint32_t index = vaddress / VM_PAGE_SIZE;

	vm_page_table_t pageTable;
	vm_address_t kernelPage;

	if(!(directory[index / VM_DIRECTORY_LENGTH] & VM_PAGETABLEFLAG_PRESENT))
	{
		pageTable = (vm_page_table_t)pm_alloc(1);
		directory[index / VM_DIRECTORY_LENGTH] = ((uint32_t)pageTable) | VM_FLAGS_USERLAND;

		if(directory == __vm_kernelDirectory)
		{
			if(!__vm_usePhysicalKernelPages)
				pageTable = (vm_page_table_t)(VM_KERNEL_PAGE_TABLES + ((sizeof(uint32_t) * index) & ~0xFFF));
		}
		else
		{
			// Map the page temporary into memory to avoid bad access
			kernelPage = __vm_findFreePages__kernel(1, VM_LOWER_LIMIT, VM_UPPER_LIMIT);
			__vm_mapPage__noLock(__vm_kernelDirectory, (vm_address_t)pageTable, kernelPage, VM_FLAGS_KERNEL);

			pageTable = (vm_page_table_t)kernelPage;
		}
		
		memset(pageTable, 0, VM_PAGETABLE_LENGTH);
	}
	else
	{
		if(directory != __vm_kernelDirectory)
		{
			pageTable  = (vm_page_table_t)(directory[index / VM_DIRECTORY_LENGTH] & ~0xFFF);

			kernelPage = __vm_findFreePages__kernel(1, VM_LOWER_LIMIT, VM_UPPER_LIMIT);
			__vm_mapPage__noLock(__vm_kernelDirectory, (vm_address_t)pageTable, kernelPage, VM_FLAGS_KERNEL);

			pageTable = (vm_page_table_t)kernelPage;
		}
		else
		{
			if(__vm_usePhysicalKernelPages)
			{
				pageTable = (vm_page_table_t)(directory[index / VM_DIRECTORY_LENGTH] & ~0xFFF);
			}
			else
			{
				pageTable = (vm_page_table_t)(VM_KERNEL_PAGE_TABLES + ((sizeof(uint32_t) * index) & ~0xFFF));
			}
		}
	}

	pageTable[index % VM_PAGETABLE_LENGTH] = ((uint32_t)paddress) | flags;

	if(directory != __vm_kernelDirectory)
		__vm_mapPage__noLock(__vm_kernelDirectory, 0, (vm_address_t)kernelPage, 0); // Unmap the temporary page table from the kernel
	
	invlpg((uintptr_t)vaddress);
}

void __vm_mapPageRange__noLock(vm_page_directory_t directory, uintptr_t paddress, vm_address_t vaddress, size_t pages, uint32_t flags)
{
	for(size_t i=0; i<pages; i++)
	{
		__vm_mapPage__noLock(directory, paddress, vaddress, flags);

		paddress += VM_PAGE_SIZE;
		vaddress += VM_PAGE_SIZE;
	}
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

	vaddress = (isKernelDirectory) ? __vm_findFreePages__kernel(pages, VM_LOWER_LIMIT, VM_UPPER_LIMIT) : __vm_findFreePages__user(pdirectory, pages, VM_LOWER_LIMIT, VM_UPPER_LIMIT);

	if(vaddress)
		__vm_mapPageRange__noLock(pdirectory, (vm_address_t)paddress, vaddress, pages, flags);

	if(!isKernelDirectory)
		__vm_mapPage__noLock(__vm_kernelDirectory, 0x0, (vm_address_t)pdirectory, 0);

	return vaddress;
}

uint32_t __vm_getPagetableEntry(vm_page_directory_t directory, vm_address_t vaddress)
{
	uint32_t result = 0x0;
	uint32_t index = vaddress / VM_PAGE_SIZE;

	if(!(directory[index / VM_PAGETABLE_LENGTH] & VM_PAGETABLEFLAG_PRESENT))
		return result;

	uintptr_t physPageTable = (uintptr_t)(directory[index / VM_PAGETABLE_LENGTH] & ~0xFFF);
	vm_page_table_t pageTable = (vm_page_table_t)__vm_alloc_noLock(__vm_kernelDirectory, physPageTable, 1, VM_FLAGS_KERNEL);

	if(pageTable[index % VM_PAGETABLE_LENGTH] & VM_PAGETABLEFLAG_PRESENT)
	{
		uint32_t entry = pageTable[index % VM_PAGETABLE_LENGTH];
		result = entry;
	}

	__vm_mapPage__noLock(__vm_kernelDirectory, 0x0, (vm_address_t)pageTable, 0);
	return result;
}




bool vm_fulfillDMARequest(dma_t *dma)
{
	vm_lock();

	vm_address_t address = __vm_findFreePages__kernel(dma->pages, VM_LOWER_LIMIT, VM_UPPER_LIMIT);
	if(address)
	{
		dma->vaddress = address;

		for(size_t i=0; i<dma->pfragmentCount; i++)
		{
			__vm_mapPageRange__noLock(__vm_kernelDirectory, dma->pfragments[i], address, dma->pfragmentPages[i], VM_FLAGS_KERNEL);
			address += (VM_PAGE_SIZE * dma->pfragmentPages[i]);
		}
	}

	vm_unlock();
	return (address != 0);
}

uintptr_t vm_resolveVirtualAddress(vm_page_directory_t directory, vm_address_t vaddress)
{	
	bool isKernelDirectory = true;

	if(directory != __vm_kernelDirectory)
	{
		directory = (vm_page_directory_t)vm_alloc(__vm_kernelDirectory, (uintptr_t)directory, 1, VM_FLAGS_KERNEL);
		isKernelDirectory = false;
	}

	vm_lock();
	uint32_t entry = __vm_getPagetableEntry(directory, vaddress);

	if(!(entry & VM_PAGETABLEFLAG_PRESENT))
	{
		if(!isKernelDirectory)
			__vm_mapPage__noLock(__vm_kernelDirectory, 0x0, (vm_address_t)directory, 0);

		vm_unlock();
		return 0x0;
	}


	if(!isKernelDirectory)
		__vm_mapPage__noLock(__vm_kernelDirectory, 0x0, (vm_address_t)directory, 0);

	vm_unlock();
	return (uintptr_t)((entry & ~0xFFF) | ((uint32_t)vaddress & 0xFFF));
}

vm_address_t vm_findFreePages_noLock(vm_page_directory_t directory, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit)
{
	vm_address_t vaddress = 0x0;
	bool isKernelDirectory = true;

	if(directory != __vm_kernelDirectory)
	{
		directory = (vm_page_directory_t)__vm_alloc_noLock(__vm_kernelDirectory, (uintptr_t)directory, 1, VM_FLAGS_KERNEL);
		isKernelDirectory = false;
	}

	if(lowerLimit < VM_UPPER_LIMIT)
		lowerLimit = VM_UPPER_LIMIT;

	vaddress = (isKernelDirectory) ? __vm_findFreePages__kernel(pages, lowerLimit, upperLimit) : __vm_findFreePages__user(directory, pages, lowerLimit, upperLimit);

	if(!isKernelDirectory)
		__vm_mapPage__noLock(__vm_kernelDirectory, 0x0, (vm_address_t)directory, 0);

	return vaddress;
}



void vm_mapPage(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags)
{
	vm_lock();
	vm_mapPage__noLock(pdirectory, paddress, vaddress, flags);
	vm_unlock();
}

void vm_mapPageRange(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t vaddress, size_t pages, uint32_t flags)
{
	vm_lock();
	vm_mapPageRange__noLock(pdirectory, paddress, vaddress, pages, flags);
	vm_unlock();
}


void vm_mapPage__noLock(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags)
{
	bool isKernelDirectory = true;

	if(pdirectory != __vm_kernelDirectory)
	{
		pdirectory = (vm_page_directory_t)__vm_alloc_noLock(__vm_kernelDirectory, (uintptr_t)pdirectory, 1, VM_FLAGS_KERNEL);
		isKernelDirectory = false;
	}

	__vm_mapPage__noLock(pdirectory, paddress, vaddress, flags);

	if(!isKernelDirectory)
		__vm_mapPage__noLock(__vm_kernelDirectory, 0x0, (vm_address_t)pdirectory, 0);
}

void vm_mapPageRange__noLock(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t vaddress, size_t pages, uint32_t flags)
{
	bool isKernelDirectory = true;

	if(pdirectory != __vm_kernelDirectory)
	{
		pdirectory = (vm_page_directory_t)__vm_alloc_noLock(__vm_kernelDirectory, (uintptr_t)pdirectory, 1, VM_FLAGS_KERNEL);
		isKernelDirectory = false;
	}

	__vm_mapPageRange__noLock(pdirectory, paddress, vaddress, pages, flags);

	if(!isKernelDirectory)
		__vm_mapPage__noLock(__vm_kernelDirectory, 0x0, (vm_address_t)pdirectory, 0);
}




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
	__vm_mapPageRange__noLock(directory, IR_TRAMPOLINE_PHYSICAL, IR_TRAMPOLINE_BEGIN, IR_TRAMPOLINE_PAGES, VM_FLAGS_KERNEL);

	// Unmap the directory
	vm_free(__vm_kernelDirectory, (vm_address_t)directory, 1);
	return (vm_page_directory_t)physPageDir;
}

void vm_deleteDirectory(vm_page_directory_t directory)
{
	vm_page_directory_t mapped = (vm_page_directory_t)vm_alloc(__vm_kernelDirectory, (uintptr_t)directory, 1, VM_FLAGS_KERNEL);

	for(int i=0; i<VM_DIRECTORY_LENGTH; i++)
	{
		uint32_t table = mapped[i] & ~VM_PAGETABLEFLAG_ALL;
		if(table)
			pm_free(table, 1);
	}

	vm_free(__vm_kernelDirectory, (vm_address_t)mapped, 1);
	pm_free((uintptr_t)directory, 1);
}


#include <libc/backtrace.h>
__noinline void vm_lock();
void vm_lock()
{
	//dbg("!{lock}\n");

	/*void *buffer[5];
	int size = backtrace(buffer, 5);

	for(int i=0; i<size; i++)
	{
		dbg(" %i: %p\n", i, buffer[i]);
	}*/

	spinlock_lock(&__vm_spinlock);
}

void vm_unlock()
{
	spinlock_unlock(&__vm_spinlock);
}


vm_address_t vm_alloc(vm_page_directory_t pdirectory, uintptr_t paddress, size_t pages, uint32_t flags)
{
	bool isKernelDirectory = true;

	if(pdirectory != __vm_kernelDirectory)
	{
		pdirectory = (vm_page_directory_t)vm_alloc(__vm_kernelDirectory, (uintptr_t)pdirectory, 1, VM_FLAGS_KERNEL);
		isKernelDirectory = false;
	}

	vm_lock();
	vm_address_t vaddress = (isKernelDirectory) ? __vm_findFreePages__kernel(pages, VM_LOWER_LIMIT, VM_UPPER_LIMIT) : __vm_findFreePages__user(pdirectory, pages, VM_LOWER_LIMIT, VM_UPPER_LIMIT);

	if(vaddress)
		__vm_mapPageRange__noLock(pdirectory, (vm_address_t)paddress, vaddress, pages, flags);

	if(!isKernelDirectory)
		__vm_mapPage__noLock(__vm_kernelDirectory, 0x0, (vm_address_t)pdirectory, 0);

	vm_unlock();
	return vaddress;
}

vm_address_t vm_allocLimit(vm_page_directory_t pdirectory, uintptr_t paddress, size_t pages, vm_address_t limit, vm_address_t upperLimit, uint32_t flags)
{
	bool isKernelDirectory = true;

	if(pdirectory != __vm_kernelDirectory)
	{
		pdirectory = (vm_page_directory_t)vm_alloc(__vm_kernelDirectory, (uintptr_t)pdirectory, 1, VM_FLAGS_KERNEL);
		isKernelDirectory = false;
	}

	vm_lock();
	vm_address_t vaddress = (isKernelDirectory) ? __vm_findFreePages__kernel(pages, limit, upperLimit) : __vm_findFreePages__user(pdirectory, pages, limit, upperLimit);

	if(vaddress)
		__vm_mapPageRange__noLock(pdirectory, (vm_address_t)paddress, vaddress, pages, flags);

	if(!isKernelDirectory)
		__vm_mapPage__noLock(__vm_kernelDirectory, 0x0, (vm_address_t)pdirectory, 0);

	vm_unlock();
	return vaddress;
}

vm_address_t vm_allocTwoSidedLimit(vm_page_directory_t pdirectory, uintptr_t paddress, size_t pages, vm_address_t limit, vm_address_t upperLimit, uint32_t flags)
{
	__vm_assert(pdirectory != __vm_kernelDirectory, "pdirectory: %p", pdirectory);

	pdirectory = (vm_page_directory_t)vm_alloc(__vm_kernelDirectory, (uintptr_t)pdirectory, 1, VM_FLAGS_KERNEL);

	vm_lock();
	vm_address_t vaddress = __vm_findFreePagesTwoSided(pdirectory, pages, limit, upperLimit);

	__vm_mapPageRange__noLock(__vm_kernelDirectory, paddress, vaddress, pages, flags);
	__vm_mapPageRange__noLock(pdirectory, paddress, vaddress, pages, flags);

	__vm_mapPage__noLock(__vm_kernelDirectory, 0x0, (vm_address_t)pdirectory, 0);

	vm_unlock();
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

	vm_lock();

	for(size_t page=0; page<pages; page++)
	{
		__vm_mapPage__noLock(pdirectory, (vm_address_t)0x0, vaddress, 0);
		vaddress += VM_PAGE_SIZE;
	}

	if(!isKernelDirectory)
		__vm_mapPage__noLock(__vm_kernelDirectory, 0x0, (vm_address_t)pdirectory, 0);

	vm_unlock();
}


// MARK: Initialization
void vm_createKernelContext()
{
	// Prepare the directory
	__vm_kernelDirectory = (vm_page_directory_t)VM_KERNEL_DIRECTORY_ADDRESS;
	memset(__vm_kernelDirectory, 0, VM_DIRECTORY_LENGTH);

	__vm_kernelDirectory[0xFF] = (uint32_t)__vm_kernelDirectory | VM_FLAGS_KERNEL;
	__vm_mapPage__noLock(__vm_kernelDirectory, (vm_address_t)__vm_kernelDirectory, (vm_address_t)__vm_kernelDirectory, VM_FLAGS_KERNEL);
}

void vm_mapMultibootModule(struct multiboot_module_s *module)
{
	// Map the module data
	uintptr_t start = VM_PAGE_ALIGN_DOWN((uintptr_t)module->start);
	uintptr_t end = VM_PAGE_ALIGN_UP((uintptr_t)module->end);

	size_t size = end - start;
	size_t pages = VM_PAGE_COUNT(size);

	__vm_mapPageRange__noLock(__vm_kernelDirectory, start, start, pages, VM_FLAGS_KERNEL);

	// Map the name of the module
	uintptr_t name = VM_PAGE_ALIGN_DOWN((uintptr_t)module->name);

	size = strlen((const char *)name);
	pages = VM_PAGE_COUNT(size);

	__vm_mapPageRange__noLock(__vm_kernelDirectory, name, name, pages, VM_FLAGS_KERNEL);
}

void vm_mapMultiboot(struct multiboot_s *info)
{
	uintptr_t infostart = VM_PAGE_ALIGN_DOWN((uintptr_t)info);
	__vm_mapPage__noLock(__vm_kernelDirectory, infostart, infostart, VM_FLAGS_KERNEL);

	if(info->flags & kMultibootFlagCommandLine)
	{
		uintptr_t address = VM_PAGE_ALIGN_DOWN((uintptr_t)info->cmdline);
		__vm_mapPage__noLock(__vm_kernelDirectory, address, address, VM_FLAGS_KERNEL);
	}

	if(info->flags & kMultibootFlagModules)
	{
		struct multiboot_module_s *module = (struct multiboot_module_s *)info->mods_addr;

		for(uint32_t i=0; i<info->mods_count; i++, module++)
		{
			vm_mapMultibootModule(module);
		}
	}

	if(info->flags & kMultibootFlagDrives)
	{
		uint8_t *drivesPtr = info->drives_addr;
		uint8_t *drivesEnd = drivesPtr + info->drives_length;

		while(drivesPtr < drivesEnd)
		{
			struct multiboot_drive_s *drive = (struct multiboot_drive_s *)drivesPtr;
			__vm_mapPageRange__noLock(__vm_kernelDirectory, (uintptr_t)drive, (vm_address_t)drive, VM_PAGE_COUNT(drive->size), VM_FLAGS_KERNEL); 

			drivesPtr += drive->size;
		}
	}

	if(info->flags & kMultibootFlagBootLoader)
	{
		uintptr_t address = VM_PAGE_ALIGN_DOWN((uintptr_t)info->boot_loader_name);
		__vm_mapPage__noLock(__vm_kernelDirectory, address, address, VM_FLAGS_KERNEL);
	}
}

extern uintptr_t stack_bottom;
extern uintptr_t stack_top;

bool vm_init(void *info)
{
	__vm_usePhysicalKernelPages = true;
	vm_createKernelContext();

	// Map the kernel into memory
	vm_address_t _kernelBegin = (vm_address_t)&kernelBegin;
	vm_address_t _kernelEnd   = (vm_address_t)&kernelEnd;

	_kernelBegin = VM_PAGE_ALIGN_DOWN(_kernelBegin);
	_kernelEnd   = VM_PAGE_ALIGN_UP(_kernelEnd);

	__vm_mapPageRange__noLock(__vm_kernelDirectory, _kernelBegin, _kernelBegin, VM_PAGE_COUNT(_kernelEnd - _kernelBegin), VM_FLAGS_KERNEL); // Map the kernel
	__vm_mapPageRange__noLock(__vm_kernelDirectory, 0xB8000, 0xB8000, 1, VM_FLAGS_KERNEL); // Map the video memory

	// Mark the kernel stack as allocated
	uintptr_t _stack_bottom = (uintptr_t)&stack_bottom;
	uintptr_t _stack_top    = (uintptr_t)&stack_top;

	_stack_bottom = VM_PAGE_ALIGN_DOWN(_stack_bottom);
	_stack_top    = VM_PAGE_ALIGN_UP(_stack_top);
	
	__vm_mapPageRange__noLock(__vm_kernelDirectory, _stack_bottom, _stack_bottom, VM_PAGE_COUNT(_stack_top - _stack_bottom), VM_FLAGS_KERNEL);

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
