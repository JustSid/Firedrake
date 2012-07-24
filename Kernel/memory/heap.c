//
//  heap.c
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

#include <libc/math.h>
#include <libc/string.h>
#include <system/assert.h>
#include <system/syslog.h>
#include <system/panic.h>
#include "memory.h"

static heap_t *_mm_kernelHeap = NULL;

heap_t *heap_create(size_t bytes, vm_page_directory_t directory, vm_address_t start, uint32_t flags)
{
	size_t nbytes = bytes + sizeof(heap_t);
	size_t npages = pageCount(nbytes);

	uintptr_t ppage  = pm_alloc(npages);
	if(!ppage)
		return NULL;
	
	vm_address_t vpage = vm_alloc(directory, ppage, npages, flags);
	if(!vpage)
	{
		pm_free(ppage, npages);
		return NULL;
	}


	heap_t *heap = (heap_t *)vpage;
	heap->size = bytes;
	heap->pages = npages;
	heap->directory = directory;
	heap->changes = 0;
	heap->firstAllocation = (struct _heap_allocation_s *)(heap + 1);

	heap->firstAllocation->size = bytes;
	heap->firstAllocation->type = kHeapAllocationTypeFree;
	heap->firstAllocation->ptr  = (uintptr_t)(heap->firstAllocation + 1);
	heap->firstAllocation->next = NULL;

	return heap;
}

heap_t *heap_kernelHeap()
{
	return _mm_kernelHeap;
}

void heap_destroy(heap_t *heap)
{
	assert(heap);

	size_t pages = heap->pages;

	vm_address_t vpage = (vm_address_t)heap;
	vm_page_directory_t directory = heap->directory;


	uintptr_t ppage = vm_resolveVirtualAddress(directory, vpage);

	pm_free(ppage, pages);
	vm_free(directory, vpage, pages);
}



size_t heap_allocationUsableSize(struct _heap_allocation_s *allocation)
{
	return allocation->size - sizeof(struct _heap_allocation_s);
}

bool heap_mergeAllocation(struct _heap_allocation_s *allocation)
{
	if(allocation->next && allocation->next->type == kHeapAllocationTypeFree)
	{
		struct _heap_allocation_s *next = allocation->next;

		allocation->size = allocation->size + next->size;
		allocation->next = next->next;

		return true;
	}

	return false;
}

void heap_divideAllocation(struct _heap_allocation_s *allocation, size_t bytes)
{
	struct _heap_allocation_s *next = (struct _heap_allocation_s *)(allocation->ptr + bytes);
	size_t psize = allocation->size;

	allocation->size = bytes + sizeof(struct _heap_allocation_s);

	next->size = psize - allocation->size;
	next->ptr = (uintptr_t)(next + 1);
	next->type = kHeapAllocationTypeFree;

	next->next = allocation->next;
	allocation->next = next;
}

struct _heap_allocation_s *heap_allocationWithPtr(heap_t *heap, void *ptr)
{
	if(!ptr)
		return NULL;

	uintptr_t tptr = (uintptr_t)ptr;
	return (struct _heap_allocation_s *)(tptr - sizeof(struct _heap_allocation_s));
}

void heap_defrag(heap_t *heap)
{
	struct _heap_allocation_s *allocation = heap->firstAllocation;
	while(allocation)
	{
		if(allocation->type == kHeapAllocationTypeFree)
		{
			while(heap_mergeAllocation(allocation)) 
			{}
		}

		allocation = allocation->next;
	}
}





void *halloc(heap_t *heap, size_t size)
{
	if(!heap)
		heap = _mm_kernelHeap;

	size = size + sizeof(struct _heap_allocation_s) + 64;

	struct _heap_allocation_s *allocation = heap->firstAllocation;
	while(allocation)
	{
		if(allocation->type == kHeapAllocationTypeFree && allocation->size >= size)
		{
			if(allocation->size > size + 32)
				heap_divideAllocation(allocation, size);

			allocation->type = kHeapAllocationTypeUsed;
			heap->changes ++;

			return (void *)allocation->ptr;
		}

		allocation = allocation->next;
	}

	return NULL;
}

void *hrealloc(heap_t *heap, void *ptr, size_t size)
{
	if(!heap)
		heap = _mm_kernelHeap;

	struct _heap_allocation_s *allocation = heap_allocationWithPtr(heap, ptr);
	if(allocation)
	{
		size_t psize = heap_allocationUsableSize(allocation);

		if(psize >= size + 128)
		{
			heap_divideAllocation(allocation, size + 64);
			return ptr;
		}

		if(psize >= size)
			return ptr;

		void *nptr = halloc(heap, size);
		if(nptr)
		{
			size_t psize = heap_allocationUsableSize(allocation);
			memcpy(ptr, nptr, psize);

			allocation->type = kHeapAllocationTypeFree;
			heap->changes ++;
		}

		return nptr;
	}

	dbg("hrealloc() with not existing ptr called!\n");
	return NULL;
}

void hfree(heap_t *heap, void *ptr)
{
	if(!heap)
		heap = _mm_kernelHeap;

	struct _heap_allocation_s *allocation = heap_allocationWithPtr(heap, ptr);
	if(allocation)
	{
		allocation->type = kHeapAllocationTypeFree;
		heap->changes ++;

		if(heap->changes > kHeapChangesThreshold)
			heap_defrag(heap);

		return;
	}

	dbg("hfree(%p), unknown pointer!\n", ptr);
}




bool heap_init(void *unused)
{
	size_t oneMb = 1024 * 1024;
	_mm_kernelHeap = heap_create(oneMb * 5, vm_getKernelDirectory(), 0, VM_FLAGS_KERNEL);

	return (_mm_kernelHeap != NULL);
}
