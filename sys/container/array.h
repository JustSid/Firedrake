//
//  array.h
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

#ifndef _ARRAY_H_
#define _ARRAY_H_

#include <types.h>
#include <system/lock.h>
#include <system/assert.h>

#include "iterator.h"

typedef struct
{
	size_t count;
	size_t space;

	size_t allocationStep;
	void **data;

	spinlock_t lock;
} array_t;

array_t *array_create();
array_t *array_copy(array_t *source);

void array_destroy(array_t *array);

void array_lock(array_t *array);
void array_unlock(array_t *array);

void array_addObject(array_t *array, void *object);
void array_insertObject(array_t *array, void *object, uint32_t index);

void array_removeObject(array_t *array, void *object);
void array_removeObjectAtIndex(array_t *array, uint32_t index);
void array_removeAllObjects(array_t *array);

uint32_t array_indexOfObject(array_t *array, void *object);
size_t array_count(array_t *array);

iterator_t *array_iterator(array_t *array);

void array_sort(array_t *array, comparator_t comparator);

static inline void *array_objectAtIndex(array_t *array, uint32_t index) 
{ 
	assert(index < array->count);
	return array->data[index]; 
}

#endif /* _ARRAY_H_ */
