//
//  virtual.cpp
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
#include <libcpp/new.h>
#include <kern/kprintf.h>
#include "virtual.h"
#include "physical.h"

#define VM_KERNEL_PAGE_TABLES 0x3fc00000

#define VM_DIRECTORY_LENGTH 1024
#define VM_PAGETABLE_LENGTH 1024

namespace vm
{
	extern "C" uint32_t *_kernel_page_directory = nullptr;

	static directory *_kernel_directory = nullptr;
	static bool _use_physical_kernel_pages;

	__inline kern_return_t find_free_pages_user(uint32_t *pageDirectory, vm_address_t &address, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);
	__inline kern_return_t find_free_pages_kernel(vm_address_t &address, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);
	__inline kern_return_t find_free_pages(uint32_t *pageDirectory, vm_address_t &address, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);
	__inline kern_return_t get_pagetable_entry(uint32_t *pageDirectory, uint32_t &entry, vm_address_t vaddress);
	__inline kern_return_t map_page_noCheck(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags);
	__inline kern_return_t map_page(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags);
	__inline kern_return_t map_page_range(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, size_t pages, uint32_t flags);

	// --------------------
	// MARK: -
	// MARK: mapped_directory
	// --------------------

	class mapped_directory
	{
	public:
		mapped_directory(uint32_t *dir) :
			_mapped(nullptr),
			_isKernelDirectory(false)
		{
			if(dir == _kernel_page_directory)
			{
				_isKernelDirectory = true;
				return;
			}

			vm_address_t address;
			kern_return_t result = _kernel_directory->alloc(address, reinterpret_cast<uintptr_t>(dir), 1, VM_FLAGS_KERNEL);

			_mapped = (result == KERN_SUCCESS) ? reinterpret_cast<uint32_t *>(address) : nullptr;
		}

		~mapped_directory()
		{
			if(!_isKernelDirectory && _mapped)
				_kernel_directory->map_page(0x0, reinterpret_cast<vm_address_t>(_mapped), 0);
		}

		uint32_t *get_directory() const
		{
			return _isKernelDirectory ? _kernel_page_directory : _mapped;
		}

	private:
		bool _isKernelDirectory;
		uint32_t *_mapped;
	};

	// --------------------
	// MARK: -
	// MARK: directory
	// --------------------

	directory::directory(uint32_t *dir) :
		_directory(dir),
		_lock(SPINLOCK_INIT)
	{}

	directory::~directory()
	{
		if(!_directory)
			return;

		vm_address_t temp;
		kern_return_t result = _kernel_directory->alloc(temp, reinterpret_cast<uintptr_t>(_directory), 1, VM_FLAGS_KERNEL);

		if(result == KERN_SUCCESS)
		{
			uint32_t *pageDirectory = reinterpret_cast<uint32_t *>(temp);

			for(size_t i = 0; i < VM_DIRECTORY_LENGTH; i ++)
			{
				uint32_t table = pageDirectory[i] & ~VM_PAGETABLEFLAG_ALL;
				if(table)
					pm::free(table, 1);
			}

			_kernel_directory->free(temp, 1);
		}

		pm::free(reinterpret_cast<uintptr_t>(_directory), 1);
	}

	kern_return_t directory::create(directory *&dir)
	{
		kern_return_t result;
		uintptr_t physical;

		result = pm::alloc(physical, 1);
		if(result != KERN_SUCCESS)
			return result;

		// Map the page directory temporarily into memory
		vm_address_t temp;
		result = _kernel_directory->alloc(temp, physical, 1, VM_FLAGS_KERNEL);
		if(result != KERN_SUCCESS)
		{
			pm::free(physical, 1);
			return result;
		}

		// Initialize the page directory
		uint32_t *pageDirectory = reinterpret_cast<uint32_t *>(temp);
		memset(pageDirectory, 0, VM_DIRECTORY_LENGTH * sizeof(uint32_t));

		pageDirectory[0xff] = (uint32_t)physical | VM_FLAGS_KERNEL;

		_kernel_directory->free(temp, 1);

		dir = new directory(reinterpret_cast<uint32_t *>(physical));
		return KERN_SUCCESS;
	}



	kern_return_t directory::map_page(uintptr_t physAddress, vm_address_t virtAddress, uint32_t flags)
	{
		spinlock_lock(&_lock);
		kern_return_t result = vm::map_page(_directory, physAddress, virtAddress, flags);
		spinlock_unlock(&_lock);

		return result;
	}
	kern_return_t directory::map_page_range(uintptr_t physAddress, vm_address_t virtAddress, size_t pages, uint32_t flags)
	{
		spinlock_lock(&_lock);
		kern_return_t result = vm::map_page_range(_directory, physAddress, virtAddress, pages, flags);
		spinlock_unlock(&_lock);

		return result;
	}
	kern_return_t directory::resolve_virtual_address(uintptr_t &physical, vm_address_t address)
	{
		mapped_directory temp(_directory);
		uint32_t *mapped = temp.get_directory();

		if(!mapped)
			return KERN_NO_MEMORY;

		uint32_t entry;
		kern_return_t result = get_pagetable_entry(mapped, entry, address);

		if(result != KERN_SUCCESS)
			return result;

		physical = (entry & ~0xfff) | (address & 0xfff);
		return KERN_SUCCESS;
	}



	kern_return_t directory::alloc(vm_address_t& address, uintptr_t physical, size_t pages, uint32_t flags)
	{
		return alloc_limit(address, physical, VM_LOWER_LIMIT, VM_UPPER_LIMIT, pages, flags);
	}

	kern_return_t directory::alloc_limit(vm_address_t& address, uintptr_t physical, vm_address_t lower, vm_address_t upper, size_t pages, uint32_t flags)
	{
		spinlock_lock(&_lock);

		mapped_directory dir(_directory);
		uint32_t *mapped = dir.get_directory();

		if(!mapped)
		{
			spinlock_unlock(&_lock);
			return KERN_NO_MEMORY;
		}

		kern_return_t result = find_free_pages(_directory, address, pages, lower, upper);
		if(result != KERN_SUCCESS)
		{
			spinlock_unlock(&_lock);
			return result;
		}

		vm::map_page_range(mapped, physical, address, pages, flags);
		spinlock_unlock(&_lock);

		return KERN_SUCCESS;
	}

	kern_return_t directory::free(vm_address_t address, size_t pages)
	{
		spinlock_lock(&_lock);

		mapped_directory dir(_directory);
		uint32_t *mapped = dir.get_directory();

		if(!mapped)
		{
			spinlock_unlock(&_lock);
			return KERN_NO_MEMORY;
		}

		vm::map_page_range(mapped, 0, address, pages, 0);
		spinlock_unlock(&_lock);

		return KERN_SUCCESS;
	}


	// --------------------
	// MARK: -
	// MARK: temporary_mapping
	// --------------------

	temporary_mapping::temporary_mapping(directory *dir, uintptr_t physical, size_t pages) :
		_dir(dir),
		_pages(pages)
	{
		kern_return_t result = _dir->alloc(_address, physical, pages, VM_FLAGS_KERNEL);
		if(result != KERN_SUCCESS)
		{
			_address = 0;
			return;
		}
	}

	temporary_mapping::temporary_mapping(directory *dir, vm_address_t &result, uintptr_t physical, size_t pages) :
		temporary_mapping(dir, physical, pages)
	{
		result = _address;
	}

	temporary_mapping::~temporary_mapping()
	{
		if(_address)
			_dir->free(_address, _pages);
	}

	// --------------------
	// MARK: -
	// MARK: Low level, non locking primitives
	// NOTE: These methods expect the appropirate locks to be held by the caller
	// --------------------

	kern_return_t find_free_pages_user(uint32_t *pageDirectory, vm_address_t &address, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit)
	{
		if((lowerLimit % VM_PAGE_SIZE) || (upperLimit % VM_PAGE_SIZE))
			return KERN_INVALID_ADDRESS;

		if(pages == 0 || lowerLimit < VM_LOWER_LIMIT || upperLimit > VM_UPPER_LIMIT)
			return KERN_INVALID_ARGUMENT;


		size_t found = 0;
		vm_address_t regionStart = 0;

		size_t pageTableIndex = (lowerLimit >> VM_DIRECTORY_SHIFT);
		size_t pageIndex = (lowerLimit >> VM_PAGE_SHIFT) % VM_PAGETABLE_LENGTH;

		temporary_mapping temp(_kernel_directory, 0xdeadbeef, 1);
		vm_address_t temporaryPage = temp.get_address();

		if(!temporaryPage)
			return KERN_NO_MEMORY;

		while(found < pages && (pageTableIndex << VM_DIRECTORY_SHIFT) < upperLimit)
		{
			if(pageTableIndex >= VM_DIRECTORY_LENGTH)
			{
				found = 0;
				break;
			}

			if(pageDirectory[pageTableIndex] & VM_PAGETABLEFLAG_PRESENT)
			{
				uint32_t *ptable = reinterpret_cast<uint32_t *>(pageDirectory[pageTableIndex] & ~0xfff);
				uint32_t *table  = reinterpret_cast<uint32_t *>(temporaryPage);

				_kernel_directory->map_page(reinterpret_cast<uintptr_t>(ptable), temporaryPage, VM_FLAGS_KERNEL);

				for(; pageIndex < VM_PAGETABLE_LENGTH; pageIndex ++)
				{
					if(!(table[pageIndex] & VM_PAGETABLEFLAG_PRESENT))
					{
						if(found == 0)
							regionStart = (pageTableIndex << VM_DIRECTORY_SHIFT) + (pageIndex << VM_PAGE_SHIFT);

						if((++ found) >= pages)
							break;
					}
					else
					{
						found = 0;
					}
				}
			}
			else
			{
				if(found == 0)
					regionStart = (pageTableIndex << VM_DIRECTORY_SHIFT) + (pageIndex << VM_PAGE_SHIFT);

				found += VM_PAGETABLE_LENGTH;
			}

			pageIndex = 0;
			pageTableIndex ++;
		}

		if(found >= pages && regionStart + (pages * VM_PAGE_SIZE) <= upperLimit)
		{
			address = regionStart;
			return KERN_SUCCESS;
		}

		return KERN_NO_MEMORY;
	}

	kern_return_t find_free_pages_kernel(vm_address_t &address, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit)
	{
		if((lowerLimit % VM_PAGE_SIZE) || (upperLimit % VM_PAGE_SIZE))
			return KERN_INVALID_ADDRESS;

		if(pages == 0 || lowerLimit < VM_LOWER_LIMIT || upperLimit > VM_UPPER_LIMIT)
			return KERN_INVALID_ARGUMENT;

		size_t found = 0;
		vm_address_t regionStart = 0;

		size_t pageTableIndex = (lowerLimit >> VM_DIRECTORY_SHIFT);
		size_t pageIndex = (lowerLimit >> VM_PAGE_SHIFT) % VM_PAGETABLE_LENGTH;

		while(found < pages && (pageTableIndex << VM_DIRECTORY_SHIFT) < upperLimit)
		{
			if(pageTableIndex >= VM_DIRECTORY_LENGTH)
			{
				found = 0;
				break;
			}

			if(_kernel_page_directory[pageTableIndex] & VM_PAGETABLEFLAG_PRESENT)
			{
				uint32_t *table = reinterpret_cast<uint32_t *>(VM_KERNEL_PAGE_TABLES + (pageTableIndex << VM_PAGE_SHIFT));

				for(; pageIndex < VM_PAGETABLE_LENGTH; pageIndex ++)
				{
					if(!(table[pageIndex] & VM_PAGETABLEFLAG_PRESENT))
					{
						if(found == 0)
							regionStart = (pageTableIndex << VM_DIRECTORY_SHIFT) + (pageIndex << VM_PAGE_SHIFT);

						if((++ found) >= pages)
							break;
					}
					else
					{
						found = 0;
					}
				}
			}
			else
			{
				if(found == 0)
					regionStart = (pageTableIndex << VM_DIRECTORY_SHIFT) + (pageIndex << VM_PAGE_SHIFT);

				found += VM_PAGETABLE_LENGTH;
			}

			pageIndex = 0;
			pageTableIndex ++;
		}

		if(found >= pages && regionStart + (pages * VM_PAGE_SIZE) <= upperLimit)
		{
			address = regionStart;
			return KERN_SUCCESS;
		}

		return KERN_NO_MEMORY;
	}

	kern_return_t find_free_pages(uint32_t *pageDirectory, vm_address_t &address, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit)
	{
		kern_return_t result;
		result = (pageDirectory == _kernel_page_directory) ? find_free_pages_kernel(address, pages, lowerLimit, upperLimit) : find_free_pages_user(pageDirectory, address, pages, lowerLimit, upperLimit);
		return result;
	}

	kern_return_t get_pagetable_entry(uint32_t *pageDirectory, uint32_t &entry, vm_address_t vaddress)
	{
		uint32_t index = vaddress / VM_PAGE_SIZE;
		if(!(pageDirectory[index / VM_PAGETABLE_LENGTH] & VM_PAGETABLEFLAG_PRESENT))
			return KERN_INVALID_ADDRESS;

		uintptr_t physPageTable = (pageDirectory[index / VM_PAGETABLE_LENGTH] & ~0xfff);
		temporary_mapping temp(_kernel_directory, physPageTable, 1);
		uint32_t *pageTable = reinterpret_cast<uint32_t *>(temp.get_address());

		if(pageTable[index % VM_PAGETABLE_LENGTH] & VM_PAGETABLEFLAG_PRESENT)
		{
			entry = pageTable[index % VM_PAGETABLE_LENGTH];
			return KERN_SUCCESS;
		}

		return KERN_INVALID_ADDRESS;
	}

	kern_return_t map_page_noCheck(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags)
	{
		uint32_t index = vaddress / VM_PAGE_SIZE;

		uint32_t *pageTable;
		vm_address_t temporaryPage = 0x0;

		if(pageDirectory != _kernel_page_directory)
		{
			kern_return_t result = _kernel_directory->alloc(temporaryPage, 0xdeadbeef, 1, VM_FLAGS_KERNEL);

			if(result != KERN_SUCCESS)
				return result;
		}

		if(!(pageDirectory[index / VM_DIRECTORY_LENGTH] & VM_PAGETABLEFLAG_PRESENT))
		{
			uintptr_t physical;
			kern_return_t result = pm::alloc(physical, 1);

			if(result != KERN_SUCCESS)
				return result;

			pageTable = reinterpret_cast<uint32_t *>(physical);
			pageDirectory[index / VM_DIRECTORY_LENGTH] = physical | VM_FLAGS_KERNEL;

			if(pageDirectory != _kernel_page_directory)
			{
				_kernel_directory->map_page(physical, temporaryPage, VM_FLAGS_KERNEL);
				pageTable = reinterpret_cast<uint32_t *>(temporaryPage);

				if(!pageTable)
				{
					pm::free(physical, 1);
					pageDirectory[index / VM_DIRECTORY_LENGTH] = 0;

					return KERN_NO_MEMORY;
				}
			}
			else if(!_use_physical_kernel_pages)
			{
				pageTable = reinterpret_cast<uint32_t *>((VM_KERNEL_PAGE_TABLES + ((sizeof(uint32_t) * index) & ~0xfff)));
			}

			memset(pageTable, 0, VM_PAGETABLE_LENGTH);
		}
		else
		{
			if(pageDirectory != _kernel_page_directory)
			{
				uintptr_t physical = (pageDirectory[index / VM_DIRECTORY_LENGTH] & ~0xfff);

				_kernel_directory->map_page(physical, temporaryPage, VM_FLAGS_KERNEL);
				pageTable = reinterpret_cast<uint32_t *>(temporaryPage);
			}
			else
			{
				uint32_t temp = _use_physical_kernel_pages ? (pageDirectory[index / VM_DIRECTORY_LENGTH] & ~0xfff) : VM_KERNEL_PAGE_TABLES + ((sizeof(uint32_t) * index) & ~0xfff);
				pageTable = reinterpret_cast<uint32_t *>(temp);
			}
		}

		pageTable[index % VM_PAGETABLE_LENGTH] = paddress | flags;

		if(temporaryPage)
			_kernel_directory->map_page(0x0, temporaryPage, 0);

		return KERN_SUCCESS;
	}

	kern_return_t map_page(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags)
	{
		if((paddress % VM_PAGE_SIZE) || (vaddress % VM_PAGE_SIZE))
			return KERN_INVALID_ADDRESS;

		if(vaddress == 0 || (paddress == 0 && flags != 0))
			return KERN_INVALID_ADDRESS;

		if(flags > VM_PAGETABLEFLAG_ALL)
			return KERN_INVALID_ARGUMENT;

		return map_page_noCheck(pageDirectory, paddress, vaddress, flags);
	}

	kern_return_t map_page_range(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, size_t pages, uint32_t flags)
	{
		if((paddress % VM_PAGE_SIZE) || (vaddress % VM_PAGE_SIZE))
			return KERN_INVALID_ADDRESS;

		if(vaddress == 0 || (paddress == 0 && flags != 0))
			return KERN_INVALID_ADDRESS;

		if(pages == 0 || flags > VM_PAGETABLEFLAG_ALL)
			return KERN_INVALID_ARGUMENT;

		for(size_t i = 0; i < pages; i ++)
		{
			// TODO: Check the return argument!
			map_page_noCheck(pageDirectory, paddress, vaddress, flags);

			vaddress += VM_PAGE_SIZE;
			paddress += VM_PAGE_SIZE;
		}

		return KERN_SUCCESS;
	}

	// --------------------
	// MARK: -
	// MARK: Accessor
	// --------------------

	directory *get_kernel_directory()
	{
		return _kernel_directory;
	}

	// --------------------
	// MARK: -
	// MARK: Initialization
	// --------------------

	extern "C" uintptr_t kernel_begin;
	extern "C" uintptr_t kernel_end;

	extern "C" uintptr_t stack_bottom;
	extern "C" uintptr_t stack_top;

	kern_return_t create_kernel_directory()
	{
		uintptr_t address;
		kern_return_t result = pm::alloc(address, 2);

		if(result != KERN_SUCCESS)
			return result;

		uint8_t *buffer = reinterpret_cast<uint8_t *>(address + VM_PAGE_SIZE);

		_kernel_page_directory = reinterpret_cast<uint32_t *>(address);
		_kernel_directory = new(buffer) directory(_kernel_page_directory);

		memset(_kernel_page_directory, 0, VM_DIRECTORY_LENGTH * sizeof(uint32_t));

		// Initialize and map the kernel directory
		_kernel_page_directory[0xff] = address | VM_FLAGS_KERNEL;
		_kernel_directory->map_page_range(address, address, 2, VM_FLAGS_KERNEL); // This call can't fail because of _use_physical_kernel_pages

		return KERN_SUCCESS;
	}

	kern_return_t init(Sys::MultibootHeader *info)
	{
		_use_physical_kernel_pages = true;

		kern_return_t result = create_kernel_directory();
		if(result != KERN_SUCCESS)
		{
			kprintf("Failed to create kernel page directory!\n");
			return result;
		}

		// Map the kernel and  video RAM 1:1
		vm_address_t kernelBegin = reinterpret_cast<vm_address_t>(&kernel_begin);
		vm_address_t kernelEnd   = reinterpret_cast<vm_address_t>(&kernel_end);

		_kernel_directory->map_page_range(kernelBegin, kernelBegin, VM_PAGE_COUNT(kernelEnd - kernelBegin), VM_FLAGS_KERNEL);
		_kernel_directory->map_page_range(0xa0000, 0xa0000, VM_PAGE_COUNT(0xc0000 - 0xa0000), VM_FLAGS_KERNEL);

		// Activate the kernel directory and virtual memory
		uint32_t cr0;
		__asm__ volatile("mov %0, %%cr3" : : "r" (reinterpret_cast<uint32_t>(_kernel_page_directory)));
		__asm__ volatile("mov %%cr0, %0" : "=r" (cr0));
		__asm__ volatile("mov %0, %%cr0" : : "r" (cr0 | (1 << 31)));

		_use_physical_kernel_pages = false;
		return KERN_SUCCESS;
	}
}
