//
//  vmemory.h
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

#ifndef _VMEMORY_H_
#define _VMEMORY_H_

#include <types.h>
#include <system/spinlock.h>

#define VM_PAGETABLEFLAG_PRESENT    0x1
#define VM_PAGETABLEFLAG_WRITEABLE  0x2
#define VM_PAGETABLEFLAG_USERSPACE  0x4

#define VM_FLAGS_KERNEL		VM_PAGETABLEFLAG_PRESENT | VM_PAGETABLEFLAG_WRITEABLE | VM_PAGETABLEFLAG_USERSPACE
#define VM_FLAGS_USERLAND	VM_PAGETABLEFLAG_PRESENT | VM_PAGETABLEFLAG_WRITEABLE | VM_PAGETABLEFLAG_USERSPACE

#define VM_DIRECTORY_LENGTH 1024
#define VM_PAGETABLE_LENGTH 1024

#define VM_SHIFT 12
#define VM_SIZE (1 << VM_SHIFT)

#define VM_KERNEL_START 0xC0000000 // TODO: Actually implement a upper half kernel...

typedef uint32_t vm_offset_t;
typedef uint32_t* vm_page_directory_t;
typedef uint32_t* vm_page_table_t;

typedef struct
{
	vm_page_directory_t directory;

	uintptr_t  directoryStart;
   	spinlock_t lock;
} vm_context_t;


vm_context_t *vm_getKernelContext();

uintptr_t vm_getPhysicalAddress(vm_context_t *context, vm_offset_t virtAddress);

bool vm_mapPage(vm_context_t *context, vm_offset_t physAddress, vm_offset_t virtAddress, uint32_t flags);
bool vm_mapPageRange(vm_context_t *context, vm_offset_t physAddress, vm_offset_t virtAddress, size_t pages, uint32_t flags);

vm_offset_t vm_alloc(vm_context_t *context, uintptr_t pmemory, size_t pages, uint32_t flags);
void 		vm_free(vm_context_t *context, vm_offset_t virtAddress, size_t pages);

void vm_activateContext(vm_context_t *context);
bool vm_init(void *ignored);

#endif /* _VMEMORY_H_ */
