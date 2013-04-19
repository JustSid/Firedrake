//
//  hashset.h
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

#ifndef _HASHSET_H_
#define _HASHSET_H_

#include <prefix.h>
#include <system/lock.h>

#include "array.h"
#include "iterator.h"

typedef struct hashset_bucket_s
{
	void *key;
	void *data;
	struct hashset_bucket_s *overflow;
} hashset_bucket_t;

typedef uint32_t (*hashset_hashfunc_t)(void *);

typedef struct
{
	size_t capacity;
	size_t count;

	hashset_bucket_t **buckets;
	hashset_hashfunc_t hashFunction;

	spinlock_t lock;
} hashset_t;

hashset_t *hashset_create(size_t capacity, hashset_hashfunc_t hashFunction);
void hashset_destroy(hashset_t *set);

array_t *hashset_allObjects(hashset_t *set);

void hashset_lock(hashset_t *set);
void hashset_unlock(hashset_t *set);

void *hashset_objectForKey(hashset_t *set, void *key);
void hashset_removeObjectForKey(hashset_t *set, void *key);
void hashset_setObjectForKey(hashset_t *set, void *data, void *key);

uint32_t hashset_count(hashset_t *set);

iterator_t *hashset_iterator(hashset_t *set);
iterator_t *hashset_keyIterator(hashset_t *set);

// Hashing functions

uint32_t hash_pointer(void *key);
uint32_t hash_cstring(void *key);

#endif /* _HASHSET_H_ */
