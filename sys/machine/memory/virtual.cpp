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

#if BOOTLOADER == BOOTLOADER_MULTIBOOT
#include <bootstrap/multiboot.h>
#endif /* BOOTLOADER == BOOTLOADER_MULTIBOOT */

namespace Sys
{
	namespace VM
	{
		constexpr vm_address_t kKernelPageTables = 0x3fc00000;
		constexpr vm_address_t kDirectoryLength  = 1024;
		constexpr vm_address_t kPagetableLength  = 1024;

		extern "C" uint32_t *_kernelPageDirectory;
		
		uint32_t *_kernelPageDirectory = nullptr;

		static Directory *_kernelDirectory = nullptr;
		static bool _usePhysicalKernelPages;

		__inline KernReturn<vm_address_t> __FindFreePagesUser(uint32_t *pageDirectory, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);
		__inline KernReturn<vm_address_t> __FindFreePagesKernel(size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);
		__inline KernReturn<vm_address_t> __FindFreePagesTwoSided(uint32_t *pageDirectory, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);
		__inline KernReturn<vm_address_t> __FindFreePages(uint32_t *pageDirectory, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit);
		__inline KernReturn<void> __MapPageNoCheck(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags, bool noLock);
		__inline KernReturn<void> __MapPage(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags, bool noLock);
		__inline KernReturn<void> __MapPageRange(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, size_t pages, uint32_t flags, bool noLock);

		// --------------------
		// MARK: -
		// MARK: ScopedDirectory
		// --------------------

		class ScopedDirectory
		{
		public:
			ScopedDirectory(uint32_t *directory) :
				_mapped(nullptr)
			{
				if(directory == _kernelPageDirectory)
				{
					_mapped = _kernelPageDirectory;
					return;
				}

				KernReturn<vm_address_t> address = _kernelDirectory->Alloc(reinterpret_cast<uintptr_t>(directory), 1, kVMFlagsKernel);
				_mapped = (address.IsValid()) ? reinterpret_cast<uint32_t *>(address.Get()) : nullptr;
			}

			~ScopedDirectory()
			{
				if(_mapped != _kernelPageDirectory)
					_kernelDirectory->MapPage(0x0, reinterpret_cast<vm_address_t>(_mapped), 0);
			}

			uint32_t *GetDirectory() const
			{
				return _mapped;
			}

		private:
			uint32_t *_mapped;
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
				_pages(pages),
				_address(0)
			{
				KernReturn<vm_address_t> result = directory->Alloc(physical, pages, kVMFlagsKernel);

				if(result.IsValid())
					_address = result.Get();
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

		KernReturn<Directory *> Directory::Create()
		{
			KernReturn<uintptr_t> physical;

			if((physical = PM::Alloc(1)).IsValid() == false)
				return physical.GetError();

			{
				ScopedDirectory scoped(reinterpret_cast<uint32_t *>(physical.Get()));
				uint32_t *mapped = scoped.GetDirectory();

				if(!mapped)
				{
					PM::Free(physical, 1);
					return Error(KERN_NO_MEMORY);
				}

				memset(mapped, 0, kDirectoryLength * sizeof(uint32_t));
				mapped[0xff] = physical | kVMFlagsKernel;
			}

			Directory *directory = Allocate<Directory>(reinterpret_cast<uint32_t *>(physical.Get()));
			if(!directory)
				return Error(KERN_NO_MEMORY);

			return directory;
		}

		Directory *Directory::GetKernelDirectory()
		{
			return _kernelDirectory;
		}

		KernReturn<void> Directory::MapPage(uintptr_t physical, vm_address_t virtAddress, Flags flags)
		{
			spinlock_lock(&_lock);
			KernReturn<void> result = __MapPage(_directory, physical, virtAddress, flags, false);
			spinlock_unlock(&_lock);

			return result;
		}

		KernReturn<void> Directory::MapPageRange(uintptr_t physical, vm_address_t virtAddress, size_t pages, Flags flags)
		{
			spinlock_lock(&_lock);
			KernReturn<void> result = __MapPageRange(_directory, physical, virtAddress, pages, flags, false);
			spinlock_unlock(&_lock);

			return result;
		}

		KernReturn<uintptr_t> Directory::ResolveAddress(vm_address_t address)
		{
			ScopedDirectory scoped(_directory);
			uint32_t *mapped = scoped.GetDirectory();

			if(!mapped)
				return Error(KERN_NO_MEMORY);

			// TODO: There really should be locking here, but that would lead to a race condition
			// and for now it works... Probably breaks one day mysteriously and I'll spend a weekend
			// trying to find out what the shit happened... Hi future me, sorry!

			KernReturn<uint32_t> entry = GetPageTableEntry(mapped, address);

			if(entry.IsValid() == false)
				return entry.GetError();

			uintptr_t resolved = (entry.Get() & ~0xfff) | (address & 0xfff);
			return resolved;
		}

		KernReturn<uint32_t> Directory::GetPageTableEntry(uint32_t *pageDirectory, vm_address_t vaddress)
		{
			uint32_t index = vaddress / VM_PAGE_SIZE;
			if(!(pageDirectory[index / kPagetableLength] & Flags::Present))
				return Error(KERN_INVALID_ADDRESS);

			uintptr_t physPageTable = (pageDirectory[index / kPagetableLength] & ~0xfff);
			ScopedMapping temp(_kernelDirectory, physPageTable, 1);

			uint32_t *pageTable = reinterpret_cast<uint32_t *>(temp.GetAddress());

			if(pageTable[index % kPagetableLength] & Flags::Present)
				return pageTable[index % kPagetableLength];

			return Error(KERN_INVALID_ADDRESS);
		}

		KernReturn<vm_address_t> Directory::Alloc(uintptr_t physical, size_t pages, Flags flags)
		{
			return AllocLimit(physical, kLowerLimit, kUpperLimit, pages, flags);
		}

		KernReturn<vm_address_t> Directory::AllocLimit(uintptr_t physical, vm_address_t lower, vm_address_t upper, size_t pages, Flags flags)
		{
			ScopedDirectory scoped(_directory);
			uint32_t *mapped = scoped.GetDirectory();

			if(!mapped)
				return Error(KERN_NO_MEMORY);


			spinlock_lock(&_lock);

			KernReturn<vm_address_t> address = __FindFreePages(mapped, pages, lower, upper);
			if(address.IsValid() == false)
			{
				spinlock_unlock(&_lock);
				return address;
			}

			__MapPageRange(mapped, physical, address, pages, flags, false);
			spinlock_unlock(&_lock);

			return address;
		}

		KernReturn<vm_address_t> Directory::AllocTwoSidedLimit(uintptr_t physical, vm_address_t lower, vm_address_t upper, size_t pages, Flags flags)
		{
			ScopedDirectory scoped(_directory);
			uint32_t *mapped = scoped.GetDirectory();

			if(!mapped)
				return Error(KERN_NO_MEMORY);


			spinlock_lock(&_lock);
			spinlock_lock(&_kernelDirectory->_lock);

			KernReturn<vm_address_t> address = __FindFreePagesTwoSided(mapped, pages, lower, upper);
			if(address.IsValid() == false)
			{
				spinlock_unlock(&_kernelDirectory->_lock);
				spinlock_unlock(&_lock);
				return address;
			}

			__MapPageRange(mapped, physical, address, pages, flags, true);
			__MapPageRange(_kernelPageDirectory, physical, address, pages, flags, true);

			spinlock_unlock(&_kernelDirectory->_lock);
			spinlock_unlock(&_lock);

			return address;
		}

		KernReturn<vm_address_t> Directory::__Alloc_NoLockPrivate(uintptr_t physical, size_t pages, Flags flags)
		{
			KernReturn<vm_address_t> address = __FindFreePages(_directory, pages, kLowerLimit, kUpperLimit);
			if(address.IsValid() == false)
				return address;

			__MapPageRange(_directory, physical, address, pages, flags, false);
			return address;
		}

		KernReturn<void> Directory::Free(vm_address_t address, size_t pages)
		{
			ScopedDirectory scoped(_directory);
			uint32_t *mapped = scoped.GetDirectory();

			if(!mapped)
				return Error(KERN_NO_MEMORY);


			spinlock_lock(&_lock);
			__MapPageRange(mapped, 0, address, pages, 0, false);
			spinlock_unlock(&_lock);
			
			return ErrorNone;	
		}

		KernReturn<vm_address_t> __FindFreePagesUser(uint32_t *pageDirectory, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit)
		{
			if((lowerLimit % VM_PAGE_SIZE) || (upperLimit % VM_PAGE_SIZE))
				return Error(KERN_INVALID_ADDRESS);

			if(pages == 0 || lowerLimit < kLowerLimit || upperLimit > kUpperLimit)
				return Error(KERN_INVALID_ARGUMENT);


			size_t found = 0;
			vm_address_t regionStart = 0;

			size_t pageTableIndex = (lowerLimit >> VM_DIRECTORY_SHIFT);
			size_t pageIndex = (lowerLimit >> VM_PAGE_SHIFT) % kPagetableLength;

			ScopedMapping temp(_kernelDirectory, 0xdeadb000, 1);
			vm_address_t temporaryPage = temp.GetAddress();

			if(!temporaryPage)
				return Error(KERN_NO_MEMORY);

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
				return regionStart;

			return Error(KERN_NO_MEMORY);
		}

		KernReturn<vm_address_t> __FindFreePagesKernel(size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit)
		{
			if((lowerLimit % VM_PAGE_SIZE) || (upperLimit % VM_PAGE_SIZE))
				return Error(KERN_INVALID_ADDRESS);

			if(pages == 0 || lowerLimit < kLowerLimit || upperLimit > kUpperLimit)
				return Error(KERN_INVALID_ARGUMENT);

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
				return regionStart;

			return Error(KERN_NO_MEMORY);
		}

		KernReturn<vm_address_t> __FindFreePagesTwoSided(uint32_t *pageDirectory, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit)
		{
			if((lowerLimit % VM_PAGE_SIZE) || (upperLimit % VM_PAGE_SIZE))
				return Error(KERN_INVALID_ADDRESS);

			if(pages == 0 || lowerLimit < kLowerLimit || upperLimit > kUpperLimit)
				return Error(KERN_INVALID_ARGUMENT);

			uint32_t pageTableIndex = (lowerLimit >> VM_DIRECTORY_SHIFT);
			uint32_t pageIndex = (lowerLimit >> VM_PAGE_SHIFT) % kPagetableLength;

			size_t foundPages = 0;
			vm_address_t regionStart = 0;

			while(foundPages < pages && (pageTableIndex << VM_DIRECTORY_SHIFT) < upperLimit)
			{
				if(pageTableIndex >= kDirectoryLength)
				{
					foundPages = 0;
					break;
				}

				if(pageDirectory[pageTableIndex] & Directory::Flags::Present || _kernelPageDirectory[pageTableIndex] & Directory::Flags::Present)
				{
					uint32_t *utable = nullptr;
					uint32_t *ktable = nullptr;

					if(pageDirectory[pageTableIndex] & Directory::Flags::Present)
					{
						uint32_t *ptable = (uint32_t *)(pageDirectory[pageTableIndex] & ~0xfff);
						KernReturn<vm_address_t> vaddress = _kernelDirectory->__Alloc_NoLockPrivate((uintptr_t)ptable, 1, kVMFlagsKernel);

						utable = (uint32_t *)vaddress.Get();
					}

					if(_kernelPageDirectory[pageTableIndex] & Directory::Flags::Present)
						ktable = (uint32_t *)(kKernelPageTables + (pageTableIndex << VM_PAGE_SHIFT));


					while(pageIndex < kPagetableLength)
					{
						bool isFree = true;

						if(utable && utable[pageIndex] & Directory::Flags::Present)
							isFree = false;

						if(ktable && ktable[pageIndex] & Directory::Flags::Present)
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
						__MapPageNoCheck(_kernelPageDirectory, 0x0, (vm_address_t)utable, 0, false);
				}
				else
				{
					if(foundPages == 0)
						regionStart = (pageTableIndex << VM_DIRECTORY_SHIFT) + (pageIndex << VM_PAGE_SHIFT);

					foundPages += kPagetableLength;
				}

				pageIndex = 0;
				pageTableIndex ++;
			}

			if(foundPages >= pages && regionStart + (pages * VM_PAGE_SIZE) <= upperLimit)
				return regionStart;

			return Error(KERN_NO_MEMORY);
		}

		KernReturn<vm_address_t> __FindFreePages(uint32_t *pageDirectory, size_t pages, vm_address_t lowerLimit, vm_address_t upperLimit)
		{
			KernReturn<vm_address_t> result = (pageDirectory == _kernelPageDirectory) ? __FindFreePagesKernel(pages, lowerLimit, upperLimit) : __FindFreePagesUser(pageDirectory, pages, lowerLimit, upperLimit);
			return result;
		}


		KernReturn<void> __MapPageNoCheck(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags, bool noLock)
		{
			uint32_t index = vaddress / VM_PAGE_SIZE;

			uint32_t *pageTable;
			vm_address_t temporaryPage = 0x0;

			if(pageDirectory != _kernelPageDirectory)
			{
				KernReturn<vm_address_t> result = noLock ? _kernelDirectory->__Alloc_NoLockPrivate(0xdeadb000, 1, kVMFlagsKernel) : _kernelDirectory->Alloc(0xdeadb000, 1, kVMFlagsKernel);

				if(result.IsValid() == false)
					return result.GetError();

				temporaryPage = result;
			}

			if(!(pageDirectory[index / kDirectoryLength] & Directory::Flags::Present))
			{
				KernReturn<uintptr_t> physical = PM::Alloc(1);

				if(physical.IsValid() == false)
					return physical.GetError();

				pageTable = reinterpret_cast<uint32_t *>(physical.Get());
				pageDirectory[index / kDirectoryLength] = physical.Get() | kVMFlagsKernel;

				if(pageDirectory != _kernelPageDirectory)
				{
					__MapPageNoCheck(_kernelPageDirectory, physical.Get(), temporaryPage, kVMFlagsKernel, false);
					pageTable = reinterpret_cast<uint32_t *>(temporaryPage);
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

					__MapPageNoCheck(_kernelPageDirectory, physical, temporaryPage, kVMFlagsKernel, false);
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
				__MapPageNoCheck(_kernelPageDirectory, 0x0, temporaryPage, 0, false);

			return ErrorNone;
		}

		KernReturn<void> __MapPage(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, uint32_t flags, bool noLock)
		{
			if((paddress % VM_PAGE_SIZE) || (vaddress % VM_PAGE_SIZE))
				return Error(KERN_INVALID_ADDRESS);

			if(vaddress == 0 || (paddress == 0 && flags != 0))
				return Error(KERN_INVALID_ADDRESS);

			if(flags > kVMFlagsAll)
				return Error(KERN_INVALID_ARGUMENT);

			return __MapPageNoCheck(pageDirectory, paddress, vaddress, flags, noLock);
		}

		KernReturn<void> __MapPageRange(uint32_t *pageDirectory, uintptr_t paddress, vm_address_t vaddress, size_t pages, uint32_t flags, bool noLock)
		{
			if((paddress % VM_PAGE_SIZE) || (vaddress % VM_PAGE_SIZE))
				return Error(KERN_INVALID_ADDRESS);

			if(vaddress == 0 || (paddress == 0 && flags != 0))
				return Error(KERN_INVALID_ADDRESS);

			if(pages == 0 || flags > kVMFlagsAll)
				return Error(KERN_INVALID_ARGUMENT);

			for(size_t i = 0; i < pages; i ++)
			{
				// TODO: Check the return argument!
				__MapPageNoCheck(pageDirectory, paddress, vaddress, flags, noLock);

				vaddress += VM_PAGE_SIZE;
				paddress += VM_PAGE_SIZE;
			}

			return ErrorNone;
		}

		// --------------------
		// MARK: -
		// MARK: Initialization
		// --------------------

		KernReturn<void> CreateKernelDirectory()
		{
			KernReturn<uintptr_t> address = PM::Alloc(2);

			if(address.IsValid() == false)
				return address.GetError();

			uint8_t *buffer = reinterpret_cast<uint8_t *>(address + VM_PAGE_SIZE);

			_kernelPageDirectory = reinterpret_cast<uint32_t *>(address.Get());
			_kernelDirectory = new(buffer) Directory(_kernelPageDirectory);

			memset(_kernelPageDirectory, 0, kDirectoryLength * sizeof(uint32_t));

			// Initialize and map the kernel directory
			_kernelPageDirectory[0xff] = address.Get() | kVMFlagsKernel;
			_kernelDirectory->MapPageRange(address, address, 2, kVMFlagsKernel); // This call can't fail because of _usePhysicalKernelPages

			return ErrorNone;
		}

#if BOOTLOADER == BOOTLOADER_MULTIBOOT
		void MarkMultibootModule(MultibootModule *module)
		{
			vm_address_t start = VM_PAGE_ALIGN_DOWN((vm_address_t)module->start);
			vm_address_t modulePage = VM_PAGE_ALIGN_DOWN((vm_address_t)module);

			size_t pages = VM_PAGE_COUNT(VM_PAGE_ALIGN_UP((vm_address_t)module->end) - start);

			_kernelDirectory->MapPage(modulePage, modulePage, kVMFlagsKernel);
			_kernelDirectory->MapPageRange(start, start, pages, kVMFlagsKernel);

			start = VM_PAGE_ALIGN_DOWN((vm_address_t)module->name);
			pages = VM_PAGE_COUNT(strlen((const char *)module->name));

			_kernelDirectory->MapPageRange(start, start, pages, kVMFlagsKernel);
		}

		void MarkMultiboot(MultibootHeader *info)
		{
			vm_address_t start = VM_PAGE_ALIGN_DOWN((uintptr_t)info);
			_kernelDirectory->MapPage(start, start, kVMFlagsKernel);

			if(info->flags & MultibootHeader::Flags::CommandLine)
			{
				start = VM_PAGE_ALIGN_DOWN((uintptr_t)info->commandLine);
				_kernelDirectory->MapPage(start, start, kVMFlagsKernel);
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
#endif
	}

	// --------------------
	// MARK: -
	// MARK: Initialization
	// --------------------

	extern "C" uintptr_t __kernel_start__;
	extern "C" uintptr_t __kernel_end__;

	KernReturn<void> VMInit()
	{
		VM::_usePhysicalKernelPages = true;

		KernReturn<void> result = VM::CreateKernelDirectory();
		if(result.IsValid() == false)
		{
			kprintf("Failed to create kernel page directory!\n");
			return result;
		}

		// Map the kernel and  video RAM 1:1
		vm_address_t kernelBegin = reinterpret_cast<vm_address_t>(&__kernel_start__);
		vm_address_t kernelEnd   = reinterpret_cast<vm_address_t>(&__kernel_end__);

		VM::_kernelDirectory->MapPageRange(kernelBegin, kernelBegin, VM_PAGE_COUNT(kernelEnd - kernelBegin), kVMFlagsKernel);
		VM::_kernelDirectory->MapPageRange(0xa0000, 0xa0000, VM_PAGE_COUNT(0xc0000 - 0xa0000), kVMFlagsKernel);

#if BOOTLOADER == BOOTLOADER_MULTIBOOT
		VM::MarkMultiboot(bootInfo);
#endif

		// Activate the kernel directory and virtual memory
		uint32_t cr0;
		__asm__ volatile("mov %0, %%cr3" : : "r" (reinterpret_cast<uint32_t>(VM::_kernelPageDirectory)));
		__asm__ volatile("mov %%cr0, %0" : "=r" (cr0));
		__asm__ volatile("mov %0, %%cr0" : : "r" (cr0 | (1 << 31)));

		VM::_usePhysicalKernelPages = false;
		return ErrorNone;
	}	
}
