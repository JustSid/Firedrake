//
//  heap.h
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

/**
 * Overview:
 * Provides a memory allocator which is better in handling non page sized allocations
 * and thus should be preferred over kalloc()!
 **/
#ifndef _HEAP_H_
#define _HEAP_H_

#include <types.h>
#include "vmemory.h"

#define kHeapChangesThreshold 5

#define kHeapAllocationTypeFree 0
#define kHeapAllocationTypeUsed 1

struct _heap_allocation_s
{
	size_t size; // Size includes the size of the allocation itself as well!
	uint8_t type;

	uintptr_t ptr;
	struct _heap_allocation_s *next;
};

typedef struct
{
	size_t size;
	size_t pages;

	uint8_t changes;
	vm_page_directory_t directory;

	struct _heap_allocation_s *firstAllocation;
} heap_t;


heap_t *heap_create(size_t bytes, vm_page_directory_t directory, vm_address_t start, uint32_t flags);
heap_t *heap_kernelHeap(); // Returns the main kernel heap

void heap_destroy(heap_t *heap);

size_t heap_allocationUsableSize(struct _heap_allocation_s *allocation);

void *halloc(heap_t *heap, size_t size);
void *hrealloc(heap_t *heap, void *ptr, size_t size);
void hfree(heap_t *heap, void *ptr);

bool heap_init(void *unused);

#endif /* _HEAP_H_ */
