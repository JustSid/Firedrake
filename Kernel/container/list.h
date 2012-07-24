//
//  list.h
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

#ifndef _LIST_H_
#define _LIST_H_

#include <types.h>

typedef struct list_base_s
{
	void *list;

	struct list_base_s *next;
	struct list_base_s *prev;
} list_base_t;

typedef struct
{
	uint32_t count;
	size_t entrySize;

	list_base_t *first;
	list_base_t *last;
} list_t;

list_t *list_create(size_t size);
void list_destroy(list_t *list);

void *list_addBack(list_t *list);
void *list_addFront(list_t *list);

void list_remove(list_t *list, void *entry);

list_base_t *list_first(list_t *list);
list_base_t *list_last(list_t *list);

#endif /* _LIST_H_ */
