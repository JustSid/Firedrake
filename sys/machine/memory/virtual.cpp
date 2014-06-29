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
#include <libc/assert.h>
#include <libcpp/new.h>
#include <kern/kprintf.h>
#include "virtual.h"
#include "physical.h"
#include "memory.h"

namespace Sys
{
	namespace VM
	{
		constexpr vm_address_t kKernelPageTables = 0x3fc00000;
		constexpr vm_address_t kDirectoryLength  = 1024;
		constexpr vm_address_t kPagetableLength  = 1024;

		extern "C" uint32_t *_kernelPageDirectory = nullptr;

		static Directory *_kernelDirectory = nullptr;
		static bool _usePhysicalKernelPages;

		__inline kern_return_t __FindFreePagesUser(uint32_t *pageDirectory, vm_address_t &address, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);
		__inline kern_return_t __FindFreePagesKernel(vm_address_t &address, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);
		__inline kern_return_t __FindFreePages(uint32_t *pageDirectory, vm_address_t &address, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);
		__inline kern_return_t __MapPageNoCheck(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags);
		__inline kern_return_t __MapPage(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags);
		__inline kern_return_t __MapPageRange(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, size_t pages, uint32_t flags);

		// --------------------
		// MARK: -
		// MARK: ScopedDirectory
		// --------------------

		class ScopedDirectory
		{
		public:
			ScopedDirectory(uint32_t *directory) :
				_mapped(nullptr),
				_isKernelDirectory(false)
			{
				if(directory == _kernelPageDirectory)
				{
					_isKernelDirectory = true;
					_mapped = _kernelPageDirectory;
					return;
				}

				vm_address_t address;
				kern_return_t result = _kernelDirectory->Alloc(address, reinterpret_cast<uintptr_t>(directory), 1, kVMFlagsKernel);

				_mapped = (result == KERN_SUCCESS) ? reinterpret_cast<uint32_t *>(address) : nullptr;
			}

			~ScopedDirectory()
			{
				if(_mapped && !_isKernelDirectory)
					_kernelDirectory->MapPage(0x0, reinterpret_cast<vm_address_t>(_mapped), 0);
			}

			uint32_t *GetDirectory() const
			{
				return _mapped;
			}

		private:
			uint32_t *_mapped;
			bool _isKernelDirectory;
		};

		// --------------------
		// MARK: -
		// MARK: ScopedMapping
		// --------------------

		class ScopedMapping
		{
		public:
			ScopedMapping(Directory *directory, uintptr_t physical, size_t pages) :
				_directory(directory),
				_pages(pages)
			{
				kern_return_t result = directory->Alloc(_address, physical, pages, kVMFlagsKernel);

				if(result != KERN_SUCCESS)
					_address = 0;
			}

			~ScopedMapping()
			{
				if(_address)
					_directory->Free(_address, _pages);
			}

			vm_address_t GetAddress() const
			{
				return _address;
			}

		private:
			Directory *_directory;
			vm_address_t _address;
			size_t _pages;
		};

		// --------------------
		// MARK: -
		// MARK: Directory
		// --------------------

		Directory::Directory(uint32_t *directory) :
			_directory(directory),
			_lock(SPINLOCK_INIT)
		{
			assert(_directory);
		}

		Directory::~Directory()
		{
			ScopedDirectory scoped(_directory);

			uint32_t *pageDirectory = scoped.GetDirectory();
			if(pageDirectory)
			{
				for(size_t i = 0; i < kDirectoryLength; i ++)
				{
					uint32_t table = pageDirectory[i] & ~kVMFlagsAll;
					if(table)
						PM::Free(table, 1);
				}
			}

			PM::Free(reinterpret_cast<uintptr_t>(_directory), 1);
		}

		kern_return_t Directory::Create(Directory *&directory)
		{
			kern_return_t result;
			uintptr_t physical;

			if((result = PM::Alloc(physical, 1)) != KERN_SUCCESS)
				return result;

			{
				ScopedDirectory scoped(reinterpret_cast<uint32_t *>(physical));
				uint32_t *mapped = scoped.GetDirectory();

				if(!mapped)
				{
					PM::Free(physical, 1);
					return KERN_NO_MEMORY;
				}

				memset(mapped, 0, kDirectoryLength * sizeof(uint32_t));
				mapped[0xff] = physical | kVMFlagsKernel;
			}

			directory = Allocate<Directory>(reinterpret_cast<uint32_t *>(physical));
			return (directory != nullptr) ? KERN_SUCCESS : KERN_NO_MEMORY;
		}

		Directory *Directory::GetKernelDirectory()
		{
			return _kernelDirectory;
		}

		kern_return_t Directory::MapPage(uintptr_t physical, vm_address_t virtAddress, Flags flags)
		{
			spinlock_lock(&_lock);
			kern_return_t result = __MapPage(_directory, physical, virtAddress, flags);
			spinlock_unlock(&_lock);

			return result;
		}

		kern_return_t Directory::MapPageRange(uintptr_t physical, vm_address_t virtAddress, size_t pages, Flags flags)
		{
			spinlock_lock(&_lock);
			kern_return_t result = __MapPageRange(_directory, physical, virtAddress, pages, flags);
			spinlock_unlock(&_lock);

			return result;
		}

		kern_return_t Directory::ResolveAddress(uintptr_t &resolved, vm_address_t address)
		{
			ScopedDirectory scoped(_directory);
			uint32_t *mapped = scoped.GetDirectory();

			if(!mapped)
				return KERN_NO_MEMORY;

			// TODO: There really should be locking here, but that would lead to a race condition
			// and for now it works... Probably breaks one day mysteriously and I'll spend a weekend
			// trying to find out what the shit happened... Hi future me, sorry!

			uint32_t entry;
			kern_return_t result = GetPageTableEntry(mapped, entry, address);

			if(resolved != KERN_SUCCESS)
				return result;

			resolved = (entry & ~0xfff) | (address & 0xfff);
			return KERN_SUCCESS;
		}

		kern_return_t Directory::GetPageTableEntry(uint32_t *pageDirectory, uint32_t &entry, vm_address_t vaddress)
		{
			uint32_t index = vaddress / VM_PAGE_SIZE;
			if(!(pageDirectory[index / kPagetableLength] & Flags::Present))
				return KERN_INVALID_ADDRESS;

			uintptr_t physPageTable = (pageDirectory[index / kPagetableLength] & ~0xfff);
			ScopedMapping temp(_kernelDirectory, physPageTable, 1);
			uint32_t *pageTable = reinterpret_cast<uint32_t *>(temp.GetAddress());

			if(pageTable[index % kPagetableLength] & Flags::Present)
			{
				entry = pageTable[index % kPagetableLength];
				return KERN_SUCCESS;
			}

			return KERN_INVALID_ADDRESS;
		}

		kern_return_t Directory::Alloc(vm_address_t &address, uintptr_t physical, size_t pages, Flags flags)
		{
			return AllocLimit(address, physical, kLowerLimit, kUpperLimit, pages, flags);
		}

		kern_return_t Directory::AllocLimit(vm_address_t &address, uintptr_t physical, vm_address_t lower, vm_address_t upper, size_t pages, Flags flags)
		{
			ScopedDirectory scoped(_directory);
			uint32_t *mapped = scoped.GetDirectory();

			if(!mapped)
				return KERN_NO_MEMORY;


			spinlock_lock(&_lock);

			kern_return_t result = __FindFreePages(_directory, address, pages, lower, upper);
			if(result != KERN_SUCCESS)
			{
				spinlock_unlock(&_lock);
				return result;
			}

			__MapPageRange(mapped, physical, address, pages, flags);
			spinlock_unlock(&_lock);

			return KERN_SUCCESS;
		}

		kern_return_t Directory::Free(vm_address_t address, size_t pages)
		{
			ScopedDirectory scoped(_directory);
			uint32_t *mapped = scoped.GetDirectory();

			if(!mapped)
				return KERN_NO_MEMORY;


			spinlock_lock(&_lock);
			__MapPageRange(mapped, 0, address, pages, 0);
			spinlock_unlock(&_lock);
			
			return KERN_SUCCESS;	
		}

		kern_return_t __FindFreePagesUser(uint32_t *pageDirectory, vm_address_t &address, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit)
		{
			if((lowerLimit % VM_PAGE_SIZE) || (upperLimit % VM_PAGE_SIZE))
				return KERN_INVALID_ADDRESS;

			if(pages == 0 || lowerLimit < kLowerLimit || upperLimit > kUpperLimit)
				return KERN_INVALID_ARGUMENT;


			size_t found = 0;
			vm_address_t regionStart = 0;

			size_t pageTableIndex = (lowerLimit >> VM_DIRECTORY_SHIFT);
			size_t pageIndex = (lowerLimit >> VM_PAGE_SHIFT) % kPagetableLength;

			ScopedMapping temp(_kernelDirectory, 0xdeadbeef, 1);
			vm_address_t temporaryPage = temp.GetAddress();

			if(!temporaryPage)
				return KERN_NO_MEMORY;

			while(found < pages && (pageTableIndex << VM_DIRECTORY_SHIFT) < upperLimit)
			{
				if(pageTableIndex >= kDirectoryLength)
				{
					found = 0;
					break;
				}

				if(pageDirectory[pageTableIndex] & Directory::Flags::Present)
				{
					uint32_t *ptable = reinterpret_cast<uint32_t *>(pageDirectory[pageTableIndex] & ~0xfff);
					uint32_t *table  = reinterpret_cast<uint32_t *>(temporaryPage);

					_kernelDirectory->MapPage(reinterpret_cast<uintptr_t>(ptable), temporaryPage, kVMFlagsKernel);

					for(; pageIndex < kPagetableLength; pageIndex ++)
					{
						if(!(table[pageIndex] & Directory::Flags::Present))
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

					found += kPagetableLength;
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

		kern_return_t __FindFreePagesKernel(vm_address_t &address, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit)
		{
			if((lowerLimit % VM_PAGE_SIZE) || (upperLimit % VM_PAGE_SIZE))
				return KERN_INVALID_ADDRESS;

			if(pages == 0 || lowerLimit < kLowerLimit || upperLimit > kUpperLimit)
				return KERN_INVALID_ARGUMENT;

			size_t found = 0;
			vm_address_t regionStart = 0;

			size_t pageTableIndex = (lowerLimit >> VM_DIRECTORY_SHIFT);
			size_t pageIndex      = (lowerLimit >> VM_PAGE_SHIFT) % kPagetableLength;

			while(found < pages && (pageTableIndex << VM_DIRECTORY_SHIFT) < upperLimit)
			{
				if(pageTableIndex >= kDirectoryLength)
				{
					found = 0;
					break;
				}

				if(_kernelPageDirectory[pageTableIndex] & Directory::Flags::Present)
				{
					uint32_t *table = reinterpret_cast<uint32_t *>(kKernelPageTables + (pageTableIndex << VM_PAGE_SHIFT));

					for(; pageIndex < kPagetableLength; pageIndex ++)
					{
						if(!(table[pageIndex] & Directory::Flags::Present))
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

					found += kPagetableLength;
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

		kern_return_t __FindFreePages(uint32_t *pageDirectory, vm_address_t &address, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit)
		{
			kern_return_t result;
			result = (pageDirectory == _kernelPageDirectory) ? __FindFreePagesKernel(address, pages, lowerLimit, upperLimit) : __FindFreePagesUser(pageDirectory, address, pages, lowerLimit, upperLimit);
			return result;
		}


		kern_return_t __MapPageNoCheck(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags)
		{
			uint32_t index = vaddress / VM_PAGE_SIZE;

			uint32_t *pageTable;
			vm_address_t temporaryPage = 0x0;

			if(pageDirectory != _kernelPageDirectory)
			{
				kern_return_t result = _kernelDirectory->Alloc(temporaryPage, 0xdeadbeef, 1, kVMFlagsKernel);

				if(result != KERN_SUCCESS)
					return result;
			}

			if(!(pageDirectory[index / kDirectoryLength] & Directory::Flags::Present))
			{
				uintptr_t physical;
				kern_return_t result = PM::Alloc(physical, 1);

				if(result != KERN_SUCCESS)
					return result;

				pageTable = reinterpret_cast<uint32_t *>(physical);
				pageDirectory[index / kDirectoryLength] = physical | kVMFlagsKernel;

				if(pageDirectory != _kernelPageDirectory)
				{
					_kernelDirectory->MapPage(physical, temporaryPage, kVMFlagsKernel);
					pageTable = reinterpret_cast<uint32_t *>(temporaryPage);

					if(!pageTable)
					{
						PM::Free(physical, 1);
						pageDirectory[index / kDirectoryLength] = 0;

						return KERN_NO_MEMORY;
					}
				}
				else if(!_usePhysicalKernelPages)
				{
					pageTable = reinterpret_cast<uint32_t *>((kKernelPageTables + ((sizeof(uint32_t) * index) & ~0xfff)));
				}

				memset(pageTable, 0, kPagetableLength);
			}
			else
			{
				if(pageDirectory != _kernelPageDirectory)
				{
					uintptr_t physical = (pageDirectory[index / kDirectoryLength] & ~0xfff);

					_kernelDirectory->MapPage(physical, temporaryPage, kVMFlagsKernel);
					pageTable = reinterpret_cast<uint32_t *>(temporaryPage);
				}
				else
				{
					uint32_t temp = _usePhysicalKernelPages ? (pageDirectory[index / kDirectoryLength] & ~0xfff) : kKernelPageTables + ((sizeof(uint32_t) * index) & ~0xfff);
					pageTable = reinterpret_cast<uint32_t *>(temp);
				}
			}

			pageTable[index % kPagetableLength] = paddress | flags;

			if(temporaryPage)
				_kernelDirectory->MapPage(0x0, temporaryPage, 0);

			return KERN_SUCCESS;
		}

		kern_return_t __MapPage(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags)
		{
			if((paddress % VM_PAGE_SIZE) || (vaddress % VM_PAGE_SIZE))
				return KERN_INVALID_ADDRESS;

			if(vaddress == 0 || (paddress == 0 && flags != 0))
				return KERN_INVALID_ADDRESS;

			if(flags > kVMFlagsAll)
				return KERN_INVALID_ARGUMENT;

			return __MapPageNoCheck(pageDirectory, paddress, vaddress, flags);
		}

		kern_return_t __MapPageRange(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, size_t pages, uint32_t flags)
		{
			if((paddress % VM_PAGE_SIZE) || (vaddress % VM_PAGE_SIZE))
				return KERN_INVALID_ADDRESS;

			if(vaddress == 0 || (paddress == 0 && flags != 0))
				return KERN_INVALID_ADDRESS;

			if(pages == 0 || flags > kVMFlagsAll)
				return KERN_INVALID_ARGUMENT;

			for(size_t i = 0; i < pages; i ++)
			{
				// TODO: Check the return argument!
				__MapPageNoCheck(pageDirectory, paddress, vaddress, flags);

				vaddress += VM_PAGE_SIZE;
				paddress += VM_PAGE_SIZE;
			}

			return KERN_SUCCESS;
		}

		// --------------------
		// MARK: -
		// MARK: Initialization
		// --------------------

		kern_return_t CreateKernelDirectory()
		{
			uintptr_t address;
			kern_return_t result = PM::Alloc(address, 2);

			if(result != KERN_SUCCESS)
				return result;

			uint8_t *buffer = reinterpret_cast<uint8_t *>(address + VM_PAGE_SIZE);

			_kernelPageDirectory = reinterpret_cast<uint32_t *>(address);
			_kernelDirectory = new(buffer) Directory(_kernelPageDirectory);

			memset(_kernelPageDirectory, 0, kDirectoryLength * sizeof(uint32_t));

			// Initialize and map the kernel directory
			_kernelPageDirectory[0xff] = address | kVMFlagsKernel;
			_kernelDirectory->MapPageRange(address, address, 2, kVMFlagsKernel); // This call can't fail because of _usePhysicalKernelPages

			return KERN_SUCCESS;
		}
	}

	// --------------------
	// MARK: -
	// MARK: Initialization
	// --------------------

	extern "C" uintptr_t kernel_begin;
	extern "C" uintptr_t kernel_end;

	kern_return_t VMInit(__unused MultibootHeader *info)
	{
		VM::_usePhysicalKernelPages = true;

		kern_return_t result = VM::CreateKernelDirectory();
		if(result != KERN_SUCCESS)
		{
			kprintf("Failed to create kernel page directory!\n");
			return result;
		}

		// Map the kernel and  video RAM 1:1
		vm_address_t kernelBegin = reinterpret_cast<vm_address_t>(&kernel_begin);
		vm_address_t kernelEnd   = reinterpret_cast<vm_address_t>(&kernel_end);

		VM::_kernelDirectory->MapPageRange(kernelBegin, kernelBegin, VM_PAGE_COUNT(kernelEnd - kernelBegin), kVMFlagsKernel);
		VM::_kernelDirectory->MapPageRange(0xa0000, 0xa0000, VM_PAGE_COUNT(0xc0000 - 0xa0000), kVMFlagsKernel);

		// Activate the kernel directory and virtual memory
		uint32_t cr0;
		__asm__ volatile("mov %0, %%cr3" : : "r" (reinterpret_cast<uint32_t>(VM::_kernelPageDirectory)));
		__asm__ volatile("mov %%cr0, %0" : "=r" (cr0));
		__asm__ volatile("mov %0, %%cr0" : : "r" (cr0 | (1 << 31)));

		VM::_usePhysicalKernelPages = false;
		return KERN_SUCCESS;
	}	
}
