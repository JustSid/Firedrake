//
//  list.c
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
#include "list.h"


list_t *list_create(size_t size)
{
	list_t *list = halloc(NULL, sizeof(list_t));
	if(list)
	{
		list->count     = 0;
		list->entrySize = size;
		list->lock      = SPINLOCK_INIT;

		list->first = list->last = NULL;
	}

	return list;
}

void list_destroy(list_t *list)
{
	list_base_t *base = list->first;
	while(base)
	{
		hfree(NULL, base);
		base = base->next;
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
	list_base_t *base = halloc(NULL, list->entrySize);
	if(base)
	{
		base->list = list;
		base->next = NULL;
		base->prev = list->last;

		list->last = base;
		list->count ++;

		if(list->first == NULL)
			list->first = base;
	}

	return base;
}

void *list_addFront(list_t *list)
{
	list_base_t *base = halloc(NULL, list->entrySize);
	if(base)
	{
		base->list = list;
		base->next = list->first;
		base->prev = NULL;

		list->first = base;
		list->count ++;

		if(list->last == NULL)
			list->last = base;
	}

	return base;
}

void list_remove(list_t *list, void *tentry)
{
	list_base_t *entry = tentry;
	if(entry->list == list)
	{
		if(entry->prev)
			entry->prev->next = entry->next;

		if(entry->next)
			entry->next->prev = entry->prev;

		if(list->first == entry)
			list->first = entry->next;

		if(list->last == entry)
			list->last = entry->prev;

		list->count --;
		hfree(NULL, entry);
	} 
}


list_base_t *list_first(list_t *list)
{
	return list->first;
}

list_base_t *list_last(list_t *list)
{
	return list->last;
}
