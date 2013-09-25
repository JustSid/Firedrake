//
//  virtual.h
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

#ifndef _VIRTUAL_H_
#define _VIRTUAL_H_

#include <libc/stdint.h>
#include <kern/kern_return.h>
#include <kern/spinlock.h>
#include <bootstrap/multiboot.h>

#define VM_PAGE_SHIFT 12
#define VM_DIRECTORY_SHIFT 22

#define VM_PAGE_SIZE (1 << VM_PAGE_SHIFT)
#define VM_PAGE_MASK (~(VM_PAGE_SIZE - 1))

#define VM_PAGE_COUNT(x)      ((((x) + ~VM_PAGE_MASK) & VM_PAGE_MASK) / VM_PAGE_SIZE)
#define VM_PAGE_ALIGN_DOWN(x) ((x) & VM_PAGE_MASK)
#define VM_PAGE_ALIGN_UP(x)   (VM_PAGE_ALIGN_DOWN((x) + ~VM_PAGE_MASK))

#define VM_LOWER_LIMIT 0x1000
#define VM_UPPER_LIMIT 0xfffff000


#define VM_PAGETABLEFLAG_PRESENT      (1 << 0)
#define VM_PAGETABLEFLAG_WRITEABLE    (1 << 1)
#define VM_PAGETABLEFLAG_USERSPACE    (1 << 2)
#define VM_PAGETABLEFLAG_WRITETHROUGH (1 << 3)
#define VM_PAGETABLEFLAG_CACHEDISABLE (1 << 4)
#define VM_PAGETABLEFLAG_ACCESSED     (1 << 5)
#define VM_PAGETABLEFLAG_DIRTY        (1 << 6)

#define VM_PAGETABLEFLAG_ALL ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6))

#define VM_FLAGS_KERNEL     (VM_PAGETABLEFLAG_PRESENT | VM_PAGETABLEFLAG_WRITEABLE)
#define VM_FLAGS_USERLAND   (VM_PAGETABLEFLAG_PRESENT | VM_PAGETABLEFLAG_WRITEABLE | VM_PAGETABLEFLAG_USERSPACE)
#define VM_FLAGS_USERLAND_R (VM_PAGETABLEFLAG_PRESENT | VM_PAGETABLEFLAG_USERSPACE)

typedef uintptr_t vm_address_t;

namespace vm
{
	class directory
	{
	public:
		directory(uint32_t *dir);
		~directory();

		kern_return_t map_page(uintptr_t physAddress, vm_address_t virtAddress, uint32_t flags);
		kern_return_t map_page_range(uintptr_t physAddress, vm_address_t virtAddress, size_t pages, uint32_t flags);
		kern_return_t resolve_virtual_address(uintptr_t &result, vm_address_t address);

		kern_return_t alloc(vm_address_t& address, uintptr_t physical, size_t pages, uint32_t flags);
		kern_return_t alloc_limit(vm_address_t& address, uintptr_t physical, vm_address_t lower, vm_address_t upper, size_t pages, uint32_t flags);
		kern_return_t free(vm_address_t address, size_t pages);

		static kern_return_t create(directory *&result);

	private:
		uint32_t *_directory;
		spinlock_t _lock;
	};

	class temporary_mapping
	{
	public:
		temporary_mapping(directory *dir, uintptr_t physical, size_t pages);
		temporary_mapping(directory *dir, vm_address_t &result, uintptr_t physical, size_t pages);
		~temporary_mapping();

		vm_address_t get_address() const { return _address; }

	private:
		directory *_dir;
		vm_address_t _address;
		size_t _pages;
	};

	static inline void invlpg(uintptr_t addr)
	{
		__asm__ volatile("invlpg (%0)" :: "r" (addr) : "memory");
	}

	directory *get_kernel_directory();
	kern_return_t init(multiboot_t *info);
}

#endif /* _VIRTUAL_H_ */
