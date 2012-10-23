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

bool heap_createSubheap(heap_t *heap, struct heap_subheap_s *subheap)
{
	uintptr_t pmemory = pm_alloc(heap->pages);
	if(!pmemory)
		return false;

	vm_address_t vmemory = vm_alloc(heap->directory, pmemory, heap->pages, VM_FLAGS_KERNEL);
	if(!vmemory)
	{
		pm_free(pmemory, heap->pages);
		return false;
	}

	subheap->changes = 0;
	subheap->initialized = true;

	subheap->pmemory = pmemory;
	subheap->vmemory = vmemory;

	subheap->pages = heap->pages;
	subheap->size  = (subheap->pages * VM_PAGE_SIZE);

	subheap->freeSize    = (subheap->pages * VM_PAGE_SIZE);
	subheap->allocations = 1;

	subheap->firstAllocation = (struct heap_allocation_s *)vmemory;
	subheap->firstAllocation->size = subheap->freeSize;
	subheap->firstAllocation->type = kHeapAllocationTypeFree;
	subheap->firstAllocation->ptr  = vmemory + sizeof(struct heap_allocation_s);

	subheap->firstAllocation->next    = NULL;
	subheap->firstAllocation->subheap = subheap;

	return true;
}

void heap_destroySubheap(heap_t *heap, struct heap_subheap_s *subheap)
{
	if(!subheap->initialized)
		return;

	subheap->firstAllocation = NULL;
	subheap->initialized = false;

	pm_free(subheap->pmemory, subheap->pages);
	vm_free(heap->directory, subheap->vmemory, subheap->pages);
}



heap_t *heap_create(size_t bytes, vm_page_directory_t directory, uint32_t flags)
{
	uintptr_t pmemory = pm_alloc(1);
	if(!pmemory)
		return NULL;
	
	vm_address_t vmemory = vm_alloc(directory, pmemory, 1, VM_FLAGS_KERNEL);
	if(!vmemory)
	{
		pm_free(pmemory, 1);
		return NULL;
	}

	heap_t *heap = (heap_t *)vmemory;
	memset(heap, 0, VM_PAGE_SIZE);

	heap->flags 	= flags;
	heap->directory = directory;
	heap->lock      = SPINLOCK_INIT;

	heap->pages    = pageCount(bytes);
	heap->size     = heap->pages * VM_PAGE_SIZE;
	heap->maxHeaps = (VM_PAGE_SIZE - sizeof(heap_t)) / sizeof(struct heap_subheap_s);
	heap->subheaps = (struct heap_subheap_s *)(vmemory + sizeof(heap_t));

	// Get the first subheap ready
	bool result = heap_createSubheap(heap, &heap->subheaps[0]);
	if(!result)
	{
		pm_free(pmemory, 1);
		vm_free(directory, vmemory, 1);

		return NULL;
	}
	
	return heap;
}

heap_t *heap_kernelHeap()
{
	return _mm_kernelHeap;
}

void heap_destroy(heap_t *heap)
{
	assert(heap);

	for(size_t i=0; i<heap->maxHeaps; i++)
	{
		heap_destroySubheap(heap, &heap->subheaps[i]);
	}

	vm_address_t vmemory = (vm_address_t)heap;
	vm_page_directory_t directory = heap->directory;

	uintptr_t pmemory = vm_resolveVirtualAddress(directory, vmemory);

	pm_free(pmemory, 1);
	vm_free(directory, vmemory, 1);
}



// Helper functions to divied and merge the heap
static inline bool heap_mergeAllocation(struct heap_allocation_s *allocation)
{
	if(allocation->next && allocation->next->type == kHeapAllocationTypeFree)
	{
		struct heap_allocation_s *next = allocation->next;

		allocation->size = allocation->size + next->size;
		allocation->next = next->next;

		return true;
	}

	return false;
}

static inline void heap_divideAllocation(struct heap_allocation_s *allocation, size_t bytes)
{
	struct heap_allocation_s *next = (struct heap_allocation_s *)(allocation->ptr + bytes);
	size_t psize = allocation->size;

	allocation->size = bytes + sizeof(struct heap_allocation_s);

	next->size = psize - allocation->size;
	next->ptr  = (uintptr_t)(next + 1);
	next->type = kHeapAllocationTypeFree;
	next->next = allocation->next;
	next->subheap = allocation->subheap;

	allocation->next = next;
}

static inline struct heap_allocation_s *heap_allocationWithPtr(heap_t *heap, void *ptr)
{
	if(!ptr)
		return NULL;

	if(heap->flags & kHeapFlagUntrusted)
	{
		uintptr_t address = (uintptr_t)ptr;

		for(size_t i=0; i<heap->maxHeaps; i++)
		{
			struct heap_subheap_s *subheap = &heap->subheaps[i];
			if(address >= subheap->vmemory && address < subheap->vmemory + subheap->size)
			{
				struct heap_allocation_s *allocation = subheap->firstAllocation;
				while(allocation)
				{
					if(allocation->ptr == address)
						return allocation;

					allocation = allocation->next;
				}

				break;
			}
		}
	}
	else
	{
		uintptr_t tptr = (uintptr_t)ptr;
		return (struct heap_allocation_s *)(tptr - sizeof(struct heap_allocation_s));
	}

	return NULL;
}



void heap_defragSubheap(struct heap_subheap_s *subheap)
{
	struct heap_allocation_s *allocation = subheap->firstAllocation;
	while(allocation)
	{
		if(allocation->type == kHeapAllocationTypeFree)
		{
			while(heap_mergeAllocation(allocation)) 
			{}
		}

		allocation = allocation->next;
	}

	subheap->changes = 0;
}

void heap_defrag(heap_t *heap)
{
	for(size_t i=0; i<heap->maxHeaps; i++)
	{
		struct heap_subheap_s *subheap = &heap->subheaps[i];
		if(subheap->initialized)
		{
			heap_defragSubheap(subheap);
		}
	}
}


struct heap_allocation_s *heap_allocateInSubheap(struct heap_subheap_s *subheap, size_t size)
{
	struct heap_allocation_s *allocation = subheap->firstAllocation;
	while(allocation)
	{
		if(allocation->type == kHeapAllocationTypeFree && allocation->size >= size)
		{
			if(allocation->size >= size + 128)
				heap_divideAllocation(allocation, size);

			allocation->type = kHeapAllocationTypeUsed;
			subheap->changes ++;
			subheap->freeSize -= allocation->size;

			return allocation;
		}

		allocation = allocation->next;
	}

	return NULL;
}

void *halloc(heap_t *heap, size_t size)
{
	if(!heap)
		heap = _mm_kernelHeap;

	spinlock_lock(&heap->lock);

	size = size + sizeof(struct heap_allocation_s);

	size_t padding = 4 - (size % 4);
	size_t asize = size + padding;

	if(asize >= heap->size)
	{
		spinlock_unlock(&heap->lock);
		return NULL;
	}

	// Look for a large enough subheap, and if none is found, create a new one
	struct heap_subheap_s *emptyHeap = NULL;

	for(size_t i=0; i<heap->maxHeaps; i++)
	{
		struct heap_subheap_s *subheap = &heap->subheaps[i];
		if(!subheap->initialized)
		{
			if(!emptyHeap)
				emptyHeap = subheap;

			continue;
		}

		if(subheap->initialized && subheap->freeSize >= asize)
		{
			struct heap_allocation_s *allocation = heap_allocateInSubheap(subheap, asize);
			assert(allocation);

			spinlock_unlock(&heap->lock);

			void *ptr = (void *)allocation->ptr;
			if(heap->flags & kHeapFlagSecure)
				memset(ptr, 0, size);

			return ptr;
		}
	}

	if(emptyHeap != NULL)
	{
		bool result = heap_createSubheap(heap, emptyHeap);
		if(result)
		{
			struct heap_allocation_s *allocation = heap_allocateInSubheap(emptyHeap, asize);
			assert(allocation);

			spinlock_unlock(&heap->lock);

			void *ptr = (void *)allocation->ptr;
			if(heap->flags & kHeapFlagSecure)
				memset(ptr, 0, size);

			return ptr;
		}
	}

	spinlock_unlock(&heap->lock);
	return NULL;
}

void hfree(heap_t *heap, void *ptr)
{
	if(!heap)
		heap = _mm_kernelHeap;

	spinlock_lock(&heap->lock);

	struct heap_allocation_s *allocation = heap_allocationWithPtr(heap, ptr);
	if(allocation)
	{
		struct heap_subheap_s *subheap = allocation->subheap;

		allocation->type = kHeapAllocationTypeFree;

		subheap->freeSize += allocation->size;
		subheap->changes ++;

		if(subheap->changes > kHeapChangesThreshold)
			heap_defragSubheap(subheap);

		spinlock_unlock(&heap->lock);
		return;
	}

	dbg("hfree(%p), unknown pointer!\n", ptr);
	spinlock_unlock(&heap->lock);
}




bool heap_init(void *UNUSED(unused))
{
	size_t oneMb = 1024 * 1024;
	_mm_kernelHeap = heap_create(oneMb * 5, vm_getKernelDirectory(), 0);

	return (_mm_kernelHeap != NULL);
}
