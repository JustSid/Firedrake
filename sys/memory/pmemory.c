//
//  pmemory.c
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

#include <system/assert.h>
#include <system/syslog.h>
#include <system/helper.h>
#include <system/panic.h>
#include <system/kernel.h>
#include <system/lock.h>
#include <interrupts/trampoline.h>
#include <libc/string.h>
#include <libc/math.h>
#include "pmemory.h"
#include "memory.h"

#define PM_HEAPSIZE 32768

extern uintptr_t kernelBegin; // Marks the beginning of the kernel (set by the linker)
extern uintptr_t kernelEnd;	// Marks the end of the kernel (also set by the linker)

extern uintptr_t stack_bottom;
extern uintptr_t stack_top;

// Bitmap of the heap.
static uint32_t __pm_heap[PM_HEAPSIZE];
static spinlock_t __pm_spinlock = SPINLOCK_INIT;

// Marks the given page as used in the heap bitmap
static inline void __pm_markUsed(uintptr_t page)
{
	uint32_t index = page / PM_PAGE_SIZE;
	__pm_heap[index / 32] &= ~(1 << (index & 31));
}

// Marks the given page as unused in the heap bitmap
static inline void __pm_markFree(uintptr_t page)
{
	uint32_t index = page / PM_PAGE_SIZE;
	__pm_heap[index / 32] |= (1 << (index & 31));
}


static inline uintptr_t __pm_findFreePages(uintptr_t lowerLimit, size_t pages)
{
	size_t found = 0; // The number of found pages
	uintptr_t page = 0; // Address of the first found page

	for(uint32_t i = lowerLimit / PM_PAGE_SIZE / 32; i<PM_HEAPSIZE; i++)
	{
		if(__pm_heap[i] == 0)
		{
			found = 0;
			continue;
		}


		if(__pm_heap[i] == 0xFFFFFFFF)
		{
			if(found == 0)
				page = i * 32 * PM_PAGE_SIZE;
 
			found += 32;
		}
		else
		{
			for(uint32_t j=0; j<32; j++)
			{
				if(__pm_heap[i] & (1 << j))
				{
					if(found == 0)
						page = (i * 32 + j) * PM_PAGE_SIZE;

					if((++ found) > pages)
						return page;
				}
				else
				{
					found = 0;
				}
			}
		}

		if(found > pages)
		{
			return page;
		}
	}

	return 0x0;
}


// MARK --
// Returns a physical page range not lower than lowerLimit
uintptr_t pm_allocLimit(uintptr_t lowerLimit, size_t pages)
{
	spinlock_lock(&__pm_spinlock);

	uintptr_t page = __pm_findFreePages(lowerLimit, pages);
	if(page)
	{
		uintptr_t temp = page;
		for(size_t i=0; i<pages; i++)
		{
			__pm_markUsed(temp);
			temp += PM_PAGE_SIZE;
		}
	}

	spinlock_unlock(&__pm_spinlock);
	return page;
}

// Returns a physical page range of n pages
uintptr_t pm_alloc(size_t pages)
{
	spinlock_lock(&__pm_spinlock);

	uintptr_t page = __pm_findFreePages(0x0, pages);
	if(page)
	{
		uintptr_t temp = page;
		for(size_t i=0; i<pages; i++)
		{
			__pm_markUsed(temp);
			temp += PM_PAGE_SIZE;
		}
	}

	spinlock_unlock(&__pm_spinlock);
	return page;
}

void pm_free(uintptr_t page, size_t pages)
{
	spinlock_lock(&__pm_spinlock);

	for(size_t i=0; i<pages; i++)
	{
		__pm_markFree(page);
		page += PM_PAGE_SIZE;
	}

	spinlock_unlock(&__pm_spinlock);
}

// MARK: Init

void pm_markMultibootModule(struct multiboot_module_s *module)
{
	// Mark the module data as used
	uintptr_t start = round4kDown((uintptr_t)module->start);
	uintptr_t end   = round4kUp((uintptr_t)module->end);

	size_t pages = pageCount((end - start));
	for(size_t i=0; i<pages; i++)
	{
		__pm_markUsed(start);
		start += PM_PAGE_SIZE;
	}

	// Mark the name of the module
	uintptr_t name = round4kDown((uintptr_t)module->name);
	pages = pageCount(strlen((const char *)name));

	for(size_t i=0; i<pages; i++)
	{
		__pm_markUsed(name);
		name += PM_PAGE_SIZE;
	}
}

void pm_markMultiboot(struct multiboot_s *info)
{
	struct multiboot_module_s *modules = (struct multiboot_module_s *)info->mods_addr;

	uintptr_t infostart = round4kDown((uintptr_t)info);
	__pm_markUsed(infostart);

	for(uint32_t i=0; i<info->mods_count; i++)
	{
		struct multiboot_module_s *module = &modules[i];
		pm_markMultibootModule(module);
	}

}

bool pm_init(void *data)
{
	assert(data);

	// Grab the information about the physical memory provided by GRUB.
	struct multiboot_s *info = (struct multiboot_s *)data;
	struct multiboot_mmap_s *mmap = info->mmap_addr;
	struct multiboot_mmap_s *mmapEnd = (void *)((uintptr_t)info->mmap_addr + info->mmap_length);
	
	size_t memoryTotal = 0; // The total amount of free memory in bytes.
	const char *suffix; // The data suffix. Eg. b, kB, Mb etc...

	
	// Initialize the heap map and memory mutex
	// The heap is initialized as everything allocated.
	memset(&__pm_heap, 0, PM_HEAPSIZE * sizeof(uint32_t));
	
	// Iterate through every memory map and mark every available memory address as free
	while(mmap < mmapEnd) 
	{
		if(mmap->type == 1) // A available memory map
		{
			uintptr_t address = mmap->base;
			uintptr_t addressEnd = address + mmap->length;
			
			// Mark the range as free
			while(address < addressEnd) 
			{
				__pm_markFree(address);
				
				address		+= PM_PAGE_SIZE;
				memoryTotal += PM_PAGE_SIZE;
			}
		}
		
		mmap ++;
	}
	
	// Mark the kernel as allocated
	uintptr_t _kernelBegin = (uintptr_t)&kernelBegin;
	uintptr_t _kernelEnd = (uintptr_t)&kernelEnd;
	
	uintptr_t address = _kernelBegin;
	while(address < _kernelEnd) 
	{
		__pm_markUsed(address);
		address += PM_PAGE_SIZE;
	}

	// Mark the kernel stack as allocated
	uintptr_t _stack_bottom = (uintptr_t)&stack_bottom;
	uintptr_t _stack_top    = (uintptr_t)&stack_top;

	_stack_bottom = round4kDown(_stack_bottom);
	_stack_top = round4kUp(_stack_top);

	while(_stack_bottom < _stack_top)
	{
		__pm_markUsed(_stack_bottom);
		_stack_bottom += PM_PAGE_SIZE;
	}

	pm_markMultiboot(info); // Oh and, we should probably mark the multiboot stuff as used? Yeah...
	__pm_markUsed(0x0); // Last but not least, lets mark NULL as used to avoid allocating it.
	
	// Try reserving space for the kernel directory
	uintptr_t directory = pm_allocLimit(VM_KERNEL_DIRECTORY_ADDRESS, 1);
	if(directory != VM_KERNEL_DIRECTORY_ADDRESS)
	{
		dbg("No kernel page directory space! But found space at %p", directory);
		return false;
	}

	// Try reserving space for the trampolines
	uintptr_t trampolines = pm_allocLimit(IR_TRAMPOLINE_PHYSICAL, IR_TRAMPOLINE_PAGES);
	if(trampolines != IR_TRAMPOLINE_PHYSICAL)
	{
		dbg("No trampoline space! But found space at %p", trampolines);
		return false;
	}

	// Print some pretty debug info
	suffix = sys_unitForSize(memoryTotal, &memoryTotal); // Calculate the largest integer quantity of the memory
	syslog(LOG_DEBUG, "%i %s RAM", memoryTotal, suffix); // And print how much RAM is available.

	return true;
}
