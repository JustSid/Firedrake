//
//  malloc.c
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

#include "sys/zone.h"
#include "sys/lock.h"
#include "math.h"
#include "string.h"
#include "stdint.h"
#include "stdio.h"

static zone_t *__malloc_zone = NULL;
static spinlock_t lock = SPINLOCK_INIT;

extern size_t __heap_sizeForPointer(zone_t *heap, uintptr_t ptr);

zone_t *_malloc_getZone()
{
	spinlock_lock(&lock);

	if(!__malloc_zone)
		__malloc_zone = _zone_create(kZoneFlagAligned);

	spinlock_unlock(&lock);

	return __malloc_zone;
}

void *malloc(size_t size)
{
	zone_t *zone = _malloc_getZone();
	return _zone_alloc(zone, size);
}

void *calloc(size_t num, size_t size)
{
	zone_t *zone = _malloc_getZone();
	void *allocation = _zone_alloc(zone, num * size);

	if(allocation)
	{
		memset(allocation, 0, num * size);
	}

	return allocation;
}

void *realloc(void *ptr, size_t size)
{
	zone_t *zone = _malloc_getZone();
	size_t oldSize = __heap_sizeForPointer(zone, (uintptr_t)ptr);

	if(oldSize > 0)
	{
		void *allocation = _zone_alloc(zone, size);

		if(allocation)
		{
			memcpy(allocation, ptr, MIN(size, oldSize));
			_zone_free(zone, ptr);

			return allocation;
		}
	}

	return NULL;
}

void free(void *ptr)
{
	zone_t *zone = _malloc_getZone();
	_zone_free(zone, ptr);
}
