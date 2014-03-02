//
//  physical.cpp
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2014 by Sidney Just
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

#include <bootstrap/multiboot.h>
#include <libc/string.h>
#include <libcpp/algorithm.h>
#include <kern/spinlock.h>
#include <kern/kprintf.h>
#include "physical.h"
#include "virtual.h"

extern "C" uintptr_t kernel_begin;
extern "C" uintptr_t kernel_end;

extern "C" uintptr_t stack_bottom;
extern "C" uintptr_t stack_top;

namespace pm
{
	static constexpr size_t heap_width = 32768;

	static uint32_t heap_bitmap[heap_width];
	static spinlock_t heap_lock = SPINLOCK_INIT;


	static inline void _mark_used(uintptr_t page)
	{
		uint32_t index = page / VM_PAGE_SIZE;
		heap_bitmap[index / 32] &= ~(1 << (index & 31));
	}

	static inline void _mark_free(uintptr_t page)
	{
		uint32_t index = page / VM_PAGE_SIZE;
		heap_bitmap[index / 32] |= (1 << (index & 31));
	}

	static inline uintptr_t _find_free_pages(size_t pages, uintptr_t lowerLimit, uintptr_t upperLimit)
	{
		size_t found = 0;
		uintptr_t page = 0;

		for(size_t i = lowerLimit / VM_PAGE_SIZE / 32; i < upperLimit / VM_PAGE_SIZE / 32; i ++)
		{
			if(heap_bitmap[i] == 0)
			{
				found = 0;
				continue;
			}

			if(heap_bitmap[i] == 0xffffffff)
			{
				if(found == 0)
					page = i * 32 * VM_PAGE_SIZE;

				found += 32;
			}
			else
			{
				for(size_t j = 0; j < 32; j ++)
				{
					if(heap_bitmap[i] & (1 << j))
					{
						if(found == 0)
							page = (i * 32 + j) * VM_PAGE_SIZE;

						if((++ found) >= pages)
							return page;
					}
					else
					{
						found = 0;
					}
				}
			}

			if(found >= pages)
				return page;
		}

		return 0x0;
	}



	kern_return_t alloc(uintptr_t& address, size_t pages)
	{
		return alloc_limit(address, pages, VM_LOWER_LIMIT, VM_UPPER_LIMIT);
	}

	kern_return_t alloc_limit(uintptr_t& address, size_t pages, uintptr_t lowerLimit, uintptr_t upperLimit)
	{
#if CONFIG_STRICT

		if(pages == 0 || lowerLimit < VM_LOWER_LIMIT || upperLimit > VM_UPPER_LIMIT)
			return KERN_INVALID_ARGUMENT;

		if((lowerLimit % VM_PAGE_SIZE) || (upperLimit % VM_PAGE_SIZE))
			return KERN_INVALID_ADDRESS;

#endif

		spinlock_lock(&heap_lock);

		uintptr_t page = _find_free_pages(pages, lowerLimit, upperLimit);
		if(page)
		{
			uintptr_t temp = page;

			for(size_t i = 0; i < pages; i ++)
			{
				_mark_used(temp);
				temp += VM_PAGE_SIZE;
			}
		}

		spinlock_unlock(&heap_lock);
		address = page;
		
		return page ? KERN_SUCCESS : KERN_NO_MEMORY;
	}

	kern_return_t free(uintptr_t page, size_t pages)
	{
#if CONFIG_STRICT

		if(page == 0 || (page % VM_PAGE_SIZE) != 0)
			return KERN_INVALID_ADDRESS;

#endif

		spinlock_lock(&heap_lock);

		for(size_t i = 0; i < pages; i ++)
		{
			_mark_free(page);
			page += VM_PAGE_SIZE;
		}

		spinlock_unlock(&heap_lock);
		return KERN_SUCCESS;
	}


	void mark_range(uintptr_t begin, uintptr_t end)
	{
		begin = VM_PAGE_ALIGN_DOWN(begin);
		end   = VM_PAGE_ALIGN_UP(end);

		while(begin < end)
		{
			_mark_used(begin);
			begin += VM_PAGE_SIZE;
		}
	}

	kern_return_t init(multiboot_t *info)
	{
		memset(heap_bitmap, 0, heap_width * sizeof(uint32_t));

		// Mark all free pages
		if(!(info->flags & kMultibootFlagMmap))
		{
			kprintf("No mmap info present!");
			return KERN_INVALID_ARGUMENT;
		}

		uint8_t *mmapPtr = reinterpret_cast<uint8_t *>(info->mmap_addr);
		uint8_t *mmapEnd = mmapPtr + info->mmap_length;

		while(mmapPtr < mmapEnd)
		{
			multiboot_mmap_t *mmap = reinterpret_cast<multiboot_mmap_t *>(mmapPtr);

			if(mmap->type == 1)
			{
				uintptr_t address    = mmap->base;
				uintptr_t addressEnd = address + mmap->length;

				// Mark the range as free
				while(address < addressEnd) 
				{
					_mark_free(address);
					address += VM_PAGE_SIZE;
				}
			}

			mmapPtr += mmap->size + 4;
		}

		// Mark the kernel as allocated
		mark_range(reinterpret_cast<uintptr_t>(&kernel_begin), reinterpret_cast<uintptr_t>(&kernel_end));

		// Mark the first megabyte as allocated as well
		// Although most of the BIOS stuff in there is undefined, it still might be useful
		// and it's up to grabs by the kernel
		mark_range(0x0, 0x100000);

		return KERN_SUCCESS;	
	}
}
