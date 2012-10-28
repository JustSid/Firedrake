//
//  array.c
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

#include <memory/memory.h>
#include <libc/string.h>
#include "array.h"

array_t *array_create()
{
	array_t *array = halloc(NULL, sizeof(array_t));
	if(array)
	{
		array->count = 0;
		array->space = 5;

		array->allocationStep = UINT32_MAX;
		array->data = halloc(NULL, array->space * sizeof(void *));

		array->lock = SPINLOCK_INIT;
	}

	return array;
}

void array_destroy(array_t *array)
{
	hfree(NULL, array->data);
	hfree(NULL, array);
}


void array_lock(array_t *array)
{
	spinlock_lock(&array->lock);
}

void array_unlock(array_t *array)
{
	spinlock_unlock(&array->lock);
}


static inline void __array_resizeToNextStepIfNeeded(array_t *array)
{
	if(array->count >= array->space - 1)
	{
		size_t space = (array->allocationStep != UINT32_MAX) ? array->space + array->allocationStep : array->space * 2;
		void **data = halloc(NULL, space * sizeof(void *));

		if(data)
		{
			memcpy(data, array->data, array->count * sizeof(void *));
			hfree(NULL, array->data);

			array->space = space;
			array->data = data;
		}
		else
		{
			panic("Failed to resize array %p from %i to %i!", array, array->space, space);
		}
	}
}

void array_addObject(array_t *array, void *object)
{
	__array_resizeToNextStepIfNeeded(array);

	array->data[array->count] = object;
	array->count ++;
}

void array_insertObject(array_t *array, void *object, uint32_t index)
{
#ifndef CONF_RELEASE
	if(index >= array->count)
	{
		panic("Array %p, insertObject(), index %i out of bounds (%i)", array, index, array->count);
		return;
	}
#endif /* CONF_RELEASE */

	__array_resizeToNextStepIfNeeded(array);

	for(uint32_t i=array->count; i>index; i--)
		array->data[i] = array->data[i - 1];

	array->data[index] = object;
	array->count ++;
}


static inline void __array_collapseIfNeeded(array_t *array)
{
	size_t space = (array->allocationStep == UINT32_MAX) ? array->space / 2 : array->space - array->allocationStep;

	// Overflow
	if(space > array->space)
		space = array->count + 5;

	if(array->count < space)
	{
		void **data = halloc(NULL, space * sizeof(void *));

		if(data)
		{
			memcpy(data, array->data, array->count * sizeof(void *));
			hfree(NULL, array->data);

			array->space = space;
			array->data = data;
		}
	}
}



void array_removeObject(array_t *array, void *object)
{
	uint32_t index = array_indexOfObject(array, object);
	array_removeObjectAtIndex(array, index);
}

void array_removeObjectAtIndex(array_t *array, uint32_t index)
{
#ifndef CONF_RELEASE
	if(index >= array->count || index == UINT32_MAX)
	{
		panic("Array %p, array_removeObjectAtIndex(), index %i out of bounds (%i)", array, index, array->count);
		return;
	}
#endif /* CONF_RELEASE */

	if(index < array->count -1)
	{
		for(uint32_t i=0; i<(array->count - index) - 1; i++)
			array->data[i + index] = array->data[i + index + 1];
	}

	array->count --;
	__array_collapseIfNeeded(array);
}

void array_removeAllObjects(array_t *array)
{
	hfree(NULL, array->data);

	array->count = 0;
	array->space = 5;

	array->data = halloc(NULL, array->space * sizeof(void *));
}

uint32_t array_indexOfObject(array_t *array, void *object)
{
	for(uint32_t i=0; i<array->count; i++)
	{
		if(array->data[i] == object)
			return i;
	}

	return UINT32_MAX;
}

size_t array_count(array_t *array)
{
	return array->count;
}


size_t array_iteratorNextObject(iterator_t *iterator, size_t maxObjects)
{
	array_t *array = iterator->data;
	uint32_t index = (uint32_t)iterator->custom[0];
	uint32_t i = 0;

	for(; i<maxObjects; i++)
	{
		if(index + i >= array->count)
			break;

		iterator->objects[i] = array->data[index + i];
	}

	iterator->custom[0] = (int32_t)(index + i);
	return i;
}

iterator_t *array_iterator(array_t *array)
{
	iterator_t *iterator = iterator_create(array_iteratorNextObject, array);
	return iterator;
}

// Sorting
static inline void __array_exchangeObjects(array_t *array, uint32_t index1, uint32_t index2)
{
	void *temp = array->data[index1];
	array->data[index1] = array->data[index2];
	array->data[index2] = temp;
}

static inline bool __array_isSorted(array_t *array, uint32_t begin, uint32_t end, comparator_t comparator)
{
	if(end == 0)
		return true;

	for(; begin<end-1; begin++)
	{
		if(comparator(array->data[begin], array->data[begin + 1]) < kCompareEqualTo)
		{
			return false;
		}
	}

	return true;
}


void __array_heapSortEx(array_t *array, uint32_t index, uint32_t length, comparator_t comparator)
{
	uint32_t index2 = (index * 2) + 1;
	void *temp = array->data[index];

	while(index2 < length)
	{
		if(index2 + 1 < length)
		{
			if(comparator(array->data[index2], array->data[index2 + 1]) < kCompareEqualTo)
				index2 ++;
		}

		if(comparator(temp, array->data[index2]) >= kCompareEqualTo)
			break;

		array->data[index] = array->data[index2];
		index = index2;
		index2 = index * 2 + 1;
	}

	array->data[index] = temp;
}

void __array_heapSort(array_t *array, size_t length, comparator_t comparator)
{
	size_t index = length / 2;
	while(index > 0)
	{
		index --;
		__array_heapSortEx(array, index, length, comparator);
	}

	if(length == 0)
		return;

	while(length - 1 > 0)
	{
		length --;

		__array_exchangeObjects(array, 0, length);
		__array_heapSortEx(array, 0, length, comparator);
	}
}


uint32_t __array_quickSortPartition(array_t *array, uint32_t begin, uint32_t end, comparator_t comparator)
{
	uint32_t pivot = begin;
	uint32_t middle = (begin + end) / 2;

	if(comparator(array->data[middle], array->data[begin]) >= kCompareEqualTo)
		pivot = middle;

	if(comparator(array->data[pivot], array->data[end]) >= kCompareEqualTo)
		pivot = end;


	__array_exchangeObjects(array, pivot, begin);
	pivot = begin;

	while(begin < end)
	{
		if(comparator(array->data[begin], array->data[end]) <= kCompareEqualTo)
		{
			__array_exchangeObjects(array, pivot, begin);
			pivot ++;
		}

		begin ++;
	}

	__array_exchangeObjects(array, pivot, end);
	return pivot;
}

void __array_quickSort(array_t *array, uint32_t begin, uint32_t end, uint32_t depth, comparator_t comparator)
{
	if(begin < end && end != UINT32_MAX)
	{
		if(depth > 0)
		{
			if(__array_isSorted(array, begin, end, comparator))
				return;

			uint32_t pivot = __array_quickSortPartition(array, begin, end, comparator);

			__array_quickSort(array, begin, pivot - 1, depth - 1, comparator);
			__array_quickSort(array, pivot + 1, end, depth - 1, comparator);
		}
		else
		{
			__array_heapSort(array, end - begin + 1, comparator);
		}
	}
}


void __array_insertionSort(array_t *array, comparator_t comparator)
{
	for(uint32_t i=1; i<array->count; i++)
	{
		uint32_t j;
		void *temp = array->data[i];

		for(j=i; j>=1; j--)
		{
			if(comparator(array->data[j - 1], temp) <= kCompareEqualTo)
				break;

			array->data[j] = array->data[j - 1];
		}

		array->data[j] = temp;
	}
}

uint32_t __array_logBase2(uint32_t count)
{
	uint32_t result = 0;
	while(count >>= 1)
	{
		result ++;
	}

	return result;
}

void array_sort(array_t *array, comparator_t comparator)
{
	if(array->count == 0)
		return;

	if(array->count > 5)
		__array_quickSort(array, 0, array->count - 1, 2 * __array_logBase2(array->count), comparator);

	__array_insertionSort(array, comparator);
}


