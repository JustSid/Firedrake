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

#include <libc/string.h>
#include <libcpp/algorithm.h>
#include <libc/sys/spinlock.h>
#include <kern/kprintf.h>
#include "physical.h"
#include "virtual.h"

#if BOOTLOADER == BOOTLOADER_MULTIBOOT
#include <bootstrap/multiboot.h>
#endif /* BOOTLOADER == BOOTLOADER_MULTIBOOT */

extern "C" uintptr_t __kernel_start__;
extern "C" uintptr_t __kernel_end__;

namespace Sys
{
	namespace PM
	{
		static constexpr size_t _heapWidth = 32768;

		static uint32_t _heapBitmap[_heapWidth];
		static spinlock_t _heapLock = SPINLOCK_INIT;


		static inline void MarkUsed(uintptr_t page)
		{
			uint32_t index = page / VM_PAGE_SIZE;
			_heapBitmap[index / 32] &= ~(INT64_C(1) << (index & 31));
		}
		static inline void MarkFree(uintptr_t page)
		{
			uint32_t index = page / VM_PAGE_SIZE;
			_heapBitmap[index / 32] |= (INT64_C(1) << (index & 31));
		}


		static inline uintptr_t FindFreePages(size_t pages, uintptr_t lowerLimit, uintptr_t upperLimit)
		{
			size_t found = 0;
			uintptr_t page = 0;

			for(size_t i = lowerLimit / VM_PAGE_SIZE / 32; i < upperLimit / VM_PAGE_SIZE / 32; i ++)
			{
				if(_heapBitmap[i] == 0)
				{
					found = 0;
					continue;
				}

				if(_heapBitmap[i] == 0xffffffff)
				{
					if(found == 0)
						page = i * 32 * VM_PAGE_SIZE;

					found += 32;
				}
				else
				{
					for(size_t j = 0; j < 32; j ++)
					{
						if(_heapBitmap[i] & (INT64_C(1) << j))
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



		KernReturn<uintptr_t> Alloc(size_t pages)
		{
			return AllocLimit(pages, kLowerLimit, kUpperLimit);
		}

		KernReturn<uintptr_t> AllocLimit(size_t pages, uintptr_t lowerLimit, uintptr_t upperLimit)
		{
			if(pages == 0 || lowerLimit < kLowerLimit || upperLimit > kUpperLimit)
				return Error(KERN_INVALID_ARGUMENT);

			if((lowerLimit % VM_PAGE_SIZE) || (upperLimit % VM_PAGE_SIZE))
				return Error(KERN_INVALID_ADDRESS);


			spinlock_lock(&_heapLock);

			uintptr_t page = FindFreePages(pages, lowerLimit, upperLimit);
			if(page)
			{
				uintptr_t temp = page;

				for(size_t i = 0; i < pages; i ++)
				{
					MarkUsed(temp);
					temp += VM_PAGE_SIZE;
				}
			}

			spinlock_unlock(&_heapLock);

			if(!page)
				return Error(KERN_NO_MEMORY);

			return page;
		}

		KernReturn<void> Free(uintptr_t page, size_t pages)
		{
			if(page == 0 || (page % VM_PAGE_SIZE) != 0)
				return Error(KERN_INVALID_ADDRESS);


			spinlock_lock(&_heapLock);

			for(size_t i = 0; i < pages; i ++)
			{
				MarkFree(page);
				page += VM_PAGE_SIZE;
			}

			spinlock_unlock(&_heapLock);
			return ErrorNone;
		}


		void MarkRange(uintptr_t begin, uintptr_t end)
		{
			begin = VM_PAGE_ALIGN_DOWN(begin);
			end   = VM_PAGE_ALIGN_UP(end);

			while(begin < end)
			{
				MarkUsed(begin);
				begin += VM_PAGE_SIZE;
			}
		}


#if BOOTLOADER == BOOTLOADER_MULTIBOOT
		void MarkMultibootModule(MultibootModule *module)
		{
			MarkUsed((uintptr_t)module->start);
			MarkRange((uintptr_t)module->start, (uintptr_t)module->end);
			MarkRange((uintptr_t)module->name, (uintptr_t)module->name + strlen((const char *)module->name));
		}

		void MarkMultiboot(MultibootHeader *info)
		{
			MarkUsed(VM_PAGE_ALIGN_DOWN((uintptr_t)info));

			if(info->flags & MultibootHeader::Flags::CommandLine)
			{
				MarkUsed(VM_PAGE_ALIGN_DOWN((uintptr_t)info->commandLine));
			}

			if(info->flags & MultibootHeader::Flags::Modules)
			{
				MultibootModule *module = info->modules;

				for(size_t i = 0; i < info->modulesCount; i ++)
				{
					MarkMultibootModule(module);
					module = module->GetNext();
				}
			}
		}
	}
#endif /* BOOTLOADER == BOOTLOADER_MULTIBOOT */

	KernReturn<void> PMInit()
	{
		memset(PM::_heapBitmap, 0, PM::_heapWidth * sizeof(uint32_t));

#if BOOTLOADER == BOOTLOADER_MULTIBOOT
		MultibootHeader *info = bootInfo;

		// Mark all free pages
		if(!(info->flags & MultibootHeader::Flags::Mmap))
		{
			kprintf("No mmap info present!");
			return Error(KERN_INVALID_ARGUMENT);
		}


		MultibootMmap *mmap = info->mmap;
		size_t count = info->GetMmapCount();

		for(size_t i = 0; i < count; i ++)
		{
			if(mmap->IsAvailable())
			{
				uintptr_t address = mmap->base;
				uintptr_t addressEnd = address + mmap->length;

				while(address < addressEnd)
				{
					PM::MarkFree(address);
					address += VM_PAGE_SIZE;
				}
			}

			mmap = mmap->GetNext();
		}
#endif /* BOOTLOADER == BOOTLOADER_MULTIBOOT */

		
		// Mark the kernel as allocated
		PM::MarkRange(reinterpret_cast<uintptr_t>(&__kernel_start__), reinterpret_cast<uintptr_t>(&__kernel_end__));

		// Mark the first megabyte as allocated as well
		// Although most of the BIOS stuff in there is undefined, it still might be useful
		// and it's up to grabs by the kernel
		PM::MarkRange(0x0, 0x100000);

#if BOOTLOADER == BOOTLOADER_MULTIBOOT
		PM::MarkMultiboot(info);
#endif /* BOOTLOADER == BOOTLOADER_MULTIBOOT */

		return ErrorNone;	
	}
}
