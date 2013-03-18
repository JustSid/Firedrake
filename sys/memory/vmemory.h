//
//  vmemory.h
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

#ifndef _VMEMORY_H_
#define _VMEMORY_H_

#include <types.h>
#include <bootstrap/multiboot.h>

#define VM_PAGETABLEFLAG_PRESENT    (1 << 0)
#define VM_PAGETABLEFLAG_WRITEABLE  (1 << 1)
#define VM_PAGETABLEFLAG_USERSPACE  (1 << 2)

#define VM_FLAGS_KERNEL		(VM_PAGETABLEFLAG_PRESENT | VM_PAGETABLEFLAG_WRITEABLE)
#define VM_FLAGS_USERLAND	(VM_PAGETABLEFLAG_PRESENT | VM_PAGETABLEFLAG_WRITEABLE | VM_PAGETABLEFLAG_USERSPACE)
#define VM_FLAGS_USERLAND_R	(VM_PAGETABLEFLAG_PRESENT | VM_PAGETABLEFLAG_USERSPACE)

#define VM_FLAGS_ALL ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5))

#define VM_DIRECTORY_LENGTH 1024
#define VM_PAGETABLE_LENGTH 1024

#define VM_SHIFT 12
#define VM_PAGE_SIZE ((uint32_t)4096)

#define VM_KERNEL_PAGE_TABLES	0x3FC00000
#define VM_KERNEL_DIRECTORY_ADDRESS 0x1000

typedef uint32_t vm_address_t;
typedef uint32_t* vm_page_directory_t;
typedef uint32_t* vm_page_table_t;

static inline void invlpg(uintptr_t addr)
{
	__asm__ volatile("invlpg (%0)" :: "r" (addr) : "memory");
}

vm_page_directory_t vm_getKernelDirectory();
vm_page_directory_t vm_createDirectory();
void vm_deleteDirectory(vm_page_directory_t directory);

uintptr_t vm_resolveVirtualAddress(vm_page_directory_t context, vm_address_t virtAddress);

bool vm_mapPage(vm_page_directory_t context, uintptr_t physAddress, vm_address_t virtAddress, uint32_t flags);
bool vm_mapPageRange(vm_page_directory_t context, uintptr_t physAddress, vm_address_t virtAddress, size_t pages, uint32_t flags);

vm_address_t vm_alloc(vm_page_directory_t context, uintptr_t pmemory, size_t pages, uint32_t flags);
vm_address_t vm_allocLimit(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t limit, size_t pages, uint32_t flags);
vm_address_t vm_allocTwoSidedLimit(vm_page_directory_t pdirectory, uintptr_t paddress, vm_address_t limit, size_t pages, uint32_t flags);

void vm_free(vm_page_directory_t context, vm_address_t virtAddress, size_t pages);

bool vm_init(void *info);

#endif /* _VMEMORY_H_ */
