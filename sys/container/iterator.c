//
//  iterator.c
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
#include "iterator.h"

iterator_t *iterator_create(iterator_fetch_t fetch, void *data)
{
	iterator_t *iterator = halloc(NULL, sizeof(iterator_t));
	if(iterator)
	{
		memset(iterator->custom, 0, 5 * sizeof(int32_t));

		iterator->data = data;
		iterator->objectCount = iterator->index = iterator->objectsLeft = 0;

		iterator->fetch = fetch;
		iterator->destroy = NULL;
	}

	return iterator;
}

void iterator_destroy(iterator_t *iterator)
{
	if(iterator->destroy)
		iterator->destroy(iterator);

	hfree(NULL, iterator);
}


void *iterator_nextObject(iterator_t *iterator)
{
	if(iterator->objectsLeft == 0)
	{
		iterator->objectsLeft = iterator->fetch(iterator, kIteratorMaxObjects);
		iterator->index = 0;

		if(iterator->objectsLeft == 0)
			return NULL;
	}

	iterator->objectsLeft --;
	
	return iterator->objects[iterator->index ++];
}
