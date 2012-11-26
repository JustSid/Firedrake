//
//  atree.c
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
#include "atree.h"

static inline atree_node_t *atree_skew(atree_node_t *node)
{
	if(node->link[0]->level == node->level && node->level > 0)
	{
		atree_node_t *temp = node->link[0];
		node->link[0] = temp->link[1];
		temp->link[1] = node;

		node = temp;
	}

	return node;
}

static inline atree_node_t *atree_split(atree_node_t *node)
{
	if(node->link[1]->link[1]->level == node->level && node->level > 0)
	{
		atree_node_t *temp = node->link[1];
		node->link[1] = temp->link[0];
		temp->link[0] = node;

		node = temp;
		node->level ++;
	}

	return node;
}


atree_t *atree_create(atree_compare_t comparator)
{
	atree_t *tree = halloc(NULL, sizeof(atree_t *));
	if(tree)
	{
		tree->root = tree->nil = halloc(NULL, sizeof(atree_node_t));
		if(!tree->root)
		{
			hfree(NULL, tree);
			return NULL;
		}

		tree->root->data    = NULL;
		tree->root->key     = NULL;
		tree->root->level   = 0;
		tree->root->link[0] = tree->root->link[1] = tree->root;

		tree->comparison = comparator;
		tree->nodes = 0;
	}

	return tree;
}

void atree_destroy(atree_t *tree)
{
	atree_node_t *node = tree->root;
	atree_node_t *temp;

	while(node != tree->nil)
	{
		if(node->link[0] == tree->nil)
		{
			temp = node->link[1];
			hfree(NULL, node);
		}
		else
		{
			temp = node->link[0];

			node->link[0] = temp->link[1];
			temp->link[1] = node;
		}

		node = temp;
	}

	hfree(NULL, tree->nil);
	hfree(NULL, tree);
}

uint32_t atree_count(atree_t *tree)
{
	return tree->nodes;
}


void *atree_find(atree_t *tree, void *key)
{
	atree_node_t *node = tree->root;
	while(node != tree->nil)
	{
		int result = tree->comparison(node->key, key);
		if(result == 0)
			return node->data;

		int direction = (result < 0) ? 1 : 0;
		node = node->link[direction];
	}

	return NULL;
}

static inline atree_node_t *atree_createNode(atree_t *tree, void *data, void *key)
{
	atree_node_t *node = halloc(NULL, sizeof(atree_node_t));
	if(!node)
		return tree->nil;

	node->level = 1;
	node->link[0] = node->link[1] = tree->nil;

	node->data = data;
	node->key  = key;

	return node;
}

void atree_insert(atree_t *tree, void *data, void *key)
{
	if(tree->root == tree->nil)
	{
		tree->root = atree_createNode(tree, data, key);
		if(tree->root == tree->nil)
			return;
	}
	else
	{
		atree_node_t *node = tree->root;
		atree_node_t *path[kAtreeMaxHeight];

		int top = 0;
		int diretion;

		while(1)
		{
			path[top ++] = node;
			diretion = (tree->comparison(node->key, key) < 0) ? 1 : 0;

			if(node->link[diretion] == tree->nil)
				break;

			node = node->link[diretion];
		}

		node->link[diretion] = atree_createNode(tree, data, key);
		if(node->link[diretion] == tree->nil)
			return;

		while(--top >= 0)
		{
			if(top != 0)
				diretion = (path[top - 1]->link[1] == path[top]) ? 1 : 0;

			path[top] = atree_skew(path[top]);
			path[top] = atree_split(path[top]);

			if(top != 0)
			{
				path[top - 1]->link[diretion] = path[top];
			}
			else
			{
				tree->root = path[top];
			}
		}
	}

	tree->nodes ++;
}

void atree_remove(atree_t *tree, void *key)
{
	if(tree->root != tree->nil)
	{
		atree_node_t *node = tree->root;
		atree_node_t *path[kAtreeMaxHeight];

		int top = 0;
		int direction;

		while(1)
		{
			path[top ++] = node;
			if(node == tree->nil)
				return;

			int result = tree->comparison(node->key, key);
			if(result == 0)
				break;

			direction = (result < 0) ? 1 : 0;
			node = node->link[direction];
		}

		if(node->link[0] == tree->nil || node->link[1] == tree->nil)
		{
			int direction2 = (node->link[0] == tree->nil) ? 1 : 0;
			if(--top != 0)
			{
				path[top - 1]->link[direction] = node->link[direction2];
			}
			else
			{
				tree->root = node->link[1];
			}

			hfree(NULL, node);
		}
		else
		{
			atree_node_t *heir = node->link[1];
			atree_node_t *prev = node;

			while(heir->link[0] != tree->nil)
			{
				path[top ++] = heir;
				prev = heir;

				heir = heir->link[0];
			}

			node->data = heir->data;
			prev->link[(prev == node) ? 1 : 0] = heir->link[1];

			hfree(NULL, heir);
		}

		while(--top >= 0)
		{
			atree_node_t *up = path[top];
			if(top != 0)
				direction = (path[top - 1]->link[1] == up) ? 1 : 0;

			if(up->link[0]->level < up->level - 1 || up->link[1]->level < up->level - 1)
			{
				if(up->link[1]->level > --up->level)
					up->link[1]->level = up->level;

				up = atree_skew(up);
				up->link[1] = atree_skew(up->link[1]);
				up->link[1]->link[1] = atree_skew(up->link[1]->link[1]);

				up = atree_split(up);
				up->link[1] = atree_split(up->link[1]);
			}

			if(top != 0)
			{
				path[top - 1]->link[direction] = up;
			}
			else
			{
				tree->root = up;
			}
		}

		tree->nodes --;
	}
}


typedef struct
{
	atree_t *tree;
	atree_node_t *node;
	atree_node_t *path[kAtreeMaxHeight];

	size_t top;
	bool first;
} atree_iterator_t;

size_t atree_iteratorNextObject(iterator_t *iterator, size_t maxObjects)
{
	size_t gathered = 0;
	while(gathered < maxObjects)
	{
		atree_iterator_t *aiterator = iterator->data;
		atree_node_t *nil = aiterator->tree->nil;

		if(aiterator->first && aiterator->node != nil)
		{
			iterator->objects[gathered ++] = aiterator->node->data;
			aiterator->first = false;
		}

		if(aiterator->node->link[1] != nil)
		{
			aiterator->path[aiterator->top ++] = aiterator->node;
			aiterator->node = aiterator->node->link[1];

			while(aiterator->node->link[0] != nil)
			{
				aiterator->path[aiterator->top ++] = aiterator->node;
				aiterator->node = aiterator->node->link[0];
			}
		}
		else
		{
			atree_node_t *node;
			do {
				if(aiterator->top == 0)
				{
					if(aiterator->node == aiterator->tree->root)
						iterator->objects[gathered ++] = aiterator->node->data;
					
					aiterator->node = nil;
					break;
				}

				node = aiterator->node;
				aiterator->node = aiterator->path[-- aiterator->top];
			} while(node == aiterator->node->link[1]);
		}

		if(aiterator->node == nil)
			break;

		iterator->objects[gathered ++] = aiterator->node->data;
	}

	return gathered;
}

void atree_iteratorDestroy(iterator_t *iterator)
{
	hfree(NULL, iterator->data);
}

iterator_t *atree_iterator(atree_t *tree)
{
	atree_iterator_t *aiterator = halloc(NULL, sizeof(atree_iterator_t));
	if(aiterator)
	{
		aiterator->tree  = tree;
		aiterator->node  = tree->root;
		aiterator->top   = 0;
		aiterator->first = true;

		if(aiterator->node != tree->nil)
		{
			while(aiterator->node->link[0] != tree->nil)
			{
				aiterator->path[aiterator->top ++] = aiterator->node;
				aiterator->node = aiterator->node->link[0];
			}
		}

		iterator_t *iterator = iterator_create(atree_iteratorNextObject, aiterator);
		iterator->destroy = atree_iteratorDestroy;

		return iterator;
	}

	return NULL;
}

