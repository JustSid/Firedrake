//
//  iterator.h
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

#ifndef _ITERATOR_H_
#define _ITERATOR_H_

#include <types.h>

struct iterator_s;

#define kIteratorMaxObjects 10

typedef size_t (*iterator_fetch_t)(struct iterator_s *iterator, size_t max);
typedef void (*iterator_destroy_t)(struct iterator_s *iterator);

typedef struct iterator_s
{
	void *data;

	void *objects[kIteratorMaxObjects];
	size_t objectCount;

	size_t index;
	size_t objectsLeft;
	
	iterator_fetch_t fetch;
	iterator_destroy_t destroy;
} iterator_t;

iterator_t *iterator_create(iterator_fetch_t fetch, void *data);
void iterator_destroy(iterator_t *iterator);

void *iterator_nextObject(iterator_t *iterator);

#endif /* _ITERATOR_H_ */
