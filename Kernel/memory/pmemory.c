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
#include <system/spinlock.h>
#include <libc/string.h>
#include "pmemory.h"

#define PM_HEAPSIZE 32768

extern uintptr_t kernelBegin; // Marks the beginning of the kernel (set by the linker)
extern uintptr_t kernelEnd;	// Marks the end of the kernel (also set by the linker)

static uint32_t 	__pm_heap[PM_HEAPSIZE];
static spinlock_t __pm_lock = SPINLOCK_INITIALIZER;


// MARK: Barebone allocator
static inline void pm_markUsed(uintptr_t page)
{
	uint32_t index = page / 4096;
	__pm_heap[index / 32] &= ~(1 << (index % 32));
}

static inline void pm_markFree(uintptr_t page)
{
	uint32_t index = page / 4096;
	__pm_heap[index / 32] |= (1 << (index % 32));
}

uintptr_t pm_alloc(size_t pages)
{
	spinlock_lock(&__pm_lock); // We must work atomic here!

	for(int i=0; i<PM_HEAPSIZE; i++)
	{
		if(__pm_heap[i])
		{
			for(int j=0; j<32; j++)
			{
				// Check if the given page is empty, if so, we check if the following pages are empty too.
				if(__pm_heap[i] & (1 << j)) 
				{
					bool isEmpty = true; // Assume that enough pages are empty
					uintptr_t page = ((i * 32 + j) * 0x1000); // The physical address to that page

					// Check if enough pages are actually free
					for(size_t k=0; k<pages; k++)
					{
						if(__pm_heap[i] & (1 << j)) 
						{
							j++;
							if(j >= 32)
							{
								j = 0;
								i++;
							}
						}
						else
						{
							// Looks like there was a non-free page inbetween...
							isEmpty = false;
							break;
						}
					}

					if(isEmpty)
					{
						// We found enough free pages, lets mark them as used!
						uintptr_t cpage = page;
						for(size_t k=0; k<pages; k++)
						{
							pm_markUsed(cpage);
							cpage += 0x1000;
						}

						spinlock_unlock(&__pm_lock);
						return page;
					}
				}
			}
		}
	}
	

	// Reaching this point means that we didn't found enough free pages. Lets log a debug message and return NULL
	spinlock_unlock(&__pm_lock);
	syslog(LOG_WARNING, "Failed to allocatd %i pages...\n", pages);

	return 0x0;
}

void pm_free(uintptr_t page, size_t pages)
{
	spinlock_lock(&__pm_lock); // We must work atomic here!

	for(size_t i=0; i<pages; i++)
	{
		pm_markFree(page);
		page += 0x1000;
	}

	spinlock_unlock(&__pm_lock);
}

// MARK: Init
bool pm_init(void *data)
{
	assert(data);

	// Grab the information about the physical memory provided by GRUB.
	struct multiboot_t *info = (struct multiboot_t *)data;
	struct multiboot_mmap_t *mmap = info->mmap_addr;
	struct multiboot_mmap_t *mmapEnd = (void *)((uintptr_t)info->mmap_addr + info->mmap_length);
	
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
				pm_markFree(address);
				
				address		+= 0x1000;
				memoryTotal += 0x1000;
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
		pm_markUsed(address);
		address += 0x1000;
	}

	pm_markUsed(0x0); // Last but not least, lets mark NULL as used to avoid allocating it.


	// Print some pretty debug info and unlock the spinlock afterwards
	suffix = sys_unitForSize(memoryTotal, &memoryTotal); // Calculate the largest integer quantity of the memory
	syslog(LOG_DEBUG, "%i %s RAM", memoryTotal, suffix); // And print how much RAM is available.

	spinlock_unlock(&__pm_lock);

	return true;
}
