//
//  list.c
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

#include <memory/memory.h>
#include <libc/string.h>
#include "list.h"

#define list_accessNext(list, entry) *((void **)&((char *)entry)[list->offsetNext])
#define list_accessPrev(list, entry) *((void **)&((char *)entry)[list->offsetPrev])

list_t *list_create(size_t size, size_t offsetNext, size_t offsetPrev)
{
	list_t *list = halloc(NULL, sizeof(list_t));
	if(list)
	{
		list->count     = 0;
		list->entrySize = size;

		list->lock      	= SPINLOCK_INIT;
		list->internalLock  = SPINLOCK_INIT;
		list->fillEntry = NULL;

		list->offsetNext = offsetNext;
		list->offsetPrev = offsetPrev;
		list->first = list->last = NULL;
	}

	return list;
}

void list_destroy(list_t *list)
{
	void *entry = list->first;
	while(entry)
	{
		void *temp = entry;
		entry = list_accessNext(list, entry);

		hfree(NULL, temp);
	}

	hfree(NULL, list);
}


void list_lock(list_t *list)
{
	spinlock_lock(&list->lock);
}

void list_unlock(list_t *list)
{
	spinlock_unlock(&list->lock);
}


void *list_addBack(list_t *list)
{
	void *entry = halloc(NULL, list->entrySize);
	if(entry)
	{
		memset(entry, 0, list->entrySize);

		if(list->fillEntry)
		{
			bool result = list->fillEntry(entry);
			if(!result)
			{
				hfree(NULL, entry);

				spinlock_unlock(&list->internalLock);
				return NULL;
			}
		}

		list_insertBack(list, entry);
	}

	return entry;
}

void *list_addFront(list_t *list)
{
	void *entry = halloc(NULL, list->entrySize);
	if(entry)
	{
		memset(entry, 0, list->entrySize);
		
		if(list->fillEntry)
		{
			bool result = list->fillEntry(entry);
			if(!result)
			{
				hfree(NULL, entry);

				spinlock_unlock(&list->internalLock);
				return NULL;
			}
		}

		list_insertFront(list, entry);
	}

	return entry;
}

void list_insertBack(list_t *list, void *entry)
{
	spinlock_lock(&list->internalLock);

	list_accessNext(list, entry) = NULL;
	list_accessPrev(list, entry) = list->last;

	if(list->last)
		list_accessNext(list, list->last) = entry;

	list->last = entry;
	list->count ++;

	if(list->first == NULL)
		list->first = entry;

	spinlock_unlock(&list->internalLock);
}

void list_insertFront(list_t *list, void *entry)
{
	spinlock_lock(&list->internalLock);

	list_accessNext(list, entry) = list->first;
	list_accessPrev(list, entry) = NULL;

	if(list->first)
		list_accessPrev(list, list->first) = entry;

	list->first = entry;
	list->count ++;

	if(list->last == NULL)
		list->last = entry;

	spinlock_unlock(&list->internalLock);
}

void list_remove(list_t *list, void *entry)
{
	spinlock_lock(&list->internalLock);
	
	void *next = list_accessNext(list, entry);
	void *prev = list_accessPrev(list, entry);

	if(prev)
		list_accessNext(list, prev) = next;

	if(next)
		list_accessPrev(list, next) = prev;

	if(list->first == entry)
		list->first = next;

	if(list->last)
		list->last = prev;

	list->count --;
	hfree(NULL, entry);

	spinlock_unlock(&list->internalLock);
}

size_t list_count(list_t *list)
{
	return list->count;
}

void *list_first(list_t *list)
{
	return list->first;
}

void *list_last(list_t *list)
{
	return list->last;
}
