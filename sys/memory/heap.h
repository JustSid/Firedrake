//
//  heap.h
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

#ifndef _HEAP_H_
#define _HEAP_H_

#include <types.h>
#include <system/lock.h>
#include "vmemory.h"

typedef enum
{
	heap_zone_typeTiny, // Max 64 bytes per entry
	heap_zone_typeSmall, // Max 256 bytes per entry
	heap_zone_typeMedium, // Max 2048 bytes per entry
	heap_zone_typeLarge // Everything else
} heap_zone_type_t;

typedef struct heap_zone_s
{
	heap_zone_type_t type;
	uint32_t changes;

	uintptr_t begin;
	uintptr_t end;

	size_t freeSize;
	size_t pages;

	size_t maxAllocations;
	size_t freeAllocations;
	size_t allocations;

	void *firstAllocation;
	void *lastAllocation;

	struct heap_zone_s *prev;
	struct heap_zone_s *next;
} heap_zone_t;


#define kHeapFlagSecure  (1 << 0)
#define kHeapFlagAligned (1 << 1)

typedef struct
{
	uint32_t flags;
	spinlock_t lock;

	heap_zone_t *firstZone;
} heap_t;


heap_t *heap_create(uint32_t flags);
void heap_destroy(heap_t *heap);

void *halloc(heap_t *heap, size_t size);
void hfree(heap_t *heap, void *ptr);

bool heap_init(void *unused);

#endif /* _HEAP_H_ */
