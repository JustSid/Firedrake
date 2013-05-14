//
//  sys/zone.h
//  libc
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

#ifndef _SYS_ZONE_H_
#define _SYS_ZONE_H_

#include "types.h"
#include "lock.h"

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
	unsigned int changes;

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


#define kZoneFlagSecure  (1 << 0)
#define kZoneFlagAligned (1 << 1)

typedef struct
{
	unsigned int flags;
	spinlock_t lock;

	heap_zone_t *firstZone;
} zone_t;


zone_t *_zone_create(unsigned int flags);
void _zone__destroy(zone_t *zone);

void *_zone_alloc(zone_t *zone, size_t size);
void _zone_free(zone_t *zone, void *ptr);

#endif /* _HEAP_H_ */
