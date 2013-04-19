//
//  atree.h
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

#ifndef _ATREE_H_
#define _ATREE_H_

#include <prefix.h>
#include "iterator.h"

#define kAtreeMaxHeight 64

typedef struct atree_node_s
{
	void *key;
	void *data;

	int level;
	struct atree_node_s *link[2];
} atree_node_t; 

typedef int (*atree_compare_t)(void *key1, void *key2);

typedef struct
{
	atree_node_t *root;
	atree_node_t *nil;

	atree_compare_t comparison;
	size_t nodes;
} atree_t;

atree_t *atree_create(atree_compare_t comparator);
void atree_destroy(atree_t *tree);

void *atree_find(atree_t *tree, void *key);

void atree_insert(atree_t *tree, void *data, void *key);
void atree_remove(atree_t *tree, void *key);

size_t atree_count(atree_t *tree);

iterator_t *atree_iterator(atree_t *tree);
iterator_t *atree_backwardsIterator(atree_t *tree);

#endif /* _AATREE_H_ */
