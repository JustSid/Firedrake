//
//  hashset.c
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
#include <system/syslog.h>
#include "hashset.h"

static size_t hashet_capacity[42] = 
{
	5, 11, 23, 41, 67, 113, 199, 317, 521, 839, 1361, 2207, 3571, 5779, 9349, 15121,
	24473, 39607, 64081, 103681, 167759, 271429, 439199, 710641, 1149857, 1860503, 3010349,
	4870843, 7881193, 12752029, 20633237, 33385273, 54018521, 87403763, 141422317, 228826121,
	370248451, 599074561, 969323023, 1568397599, 2537720629, 4106118251
};

static size_t hashset_maxCount[42] = 
{
	4, 8, 17, 29, 47, 76, 123, 199, 322, 521, 843, 1364, 2207, 3571, 5778, 9349,
	15127, 24476, 39603, 64079, 103682, 167761, 271443, 439204, 710647, 1149851, 1860498,
	3010349, 4870847, 7881196, 12752043, 20633239, 33385282, 54018521, 87403803, 141422324,
	228826127, 370248451, 599074578, 969323029, 1568397607, 2537720636
}; 


hashset_t *hashset_create(size_t capacity, hashset_hashfunc_t hashFunction)
{
	hashset_t *set = halloc(NULL, sizeof(hashset_t));
	if(set)
	{
		// Find the right capacity for the object
		for(int i=1; i<42; i++)
		{
			if(hashet_capacity[i] > capacity || i == 41)
			{
				capacity = hashet_capacity[i - 1];
				break;
			}
		}

		// Initialize the set
		set->buckets = halloc(NULL, capacity * sizeof(hashset_bucket_t **));
		if(!set->buckets)
		{
			hfree(NULL, set);
			return NULL;
		}

		memset(set->buckets, 0, capacity * sizeof(hashset_bucket_t **));

		set->capacity = capacity;
		set->count = 0;

		set->hashFunction = hashFunction;
		set->lock = SPINLOCK_INIT;
	}

	return set;
}

void hashset_destroy(hashset_t *set)
{
	for(size_t i=0; i<set->capacity; i++)
	{
		hashset_bucket_t *bucket = set->buckets[i];
		while(bucket)
		{
			hashset_bucket_t *next = bucket->overflow;
			hfree(NULL, bucket);

			bucket = next;
		}
	}

	hfree(NULL, set->buckets);
	hfree(NULL, set);
}


array_t *hashset_allData(hashset_t *set)
{
	array_t *array = array_create();

	for(size_t i=0; i<set->capacity; i++)
	{
		hashset_bucket_t *bucket = set->buckets[i];
		while(bucket)
		{
			if(bucket->data)
				array_addObject(array, bucket->data);

			bucket = bucket->overflow;
		}
	}

	return array;
}

hashset_bucket_t *hashset_findBucket1(hashset_t *set, void *key)
{
	uint32_t hash = set->hashFunction(key);
	size_t index = hash % set->capacity;

	hashset_bucket_t *bucket = set->buckets[index];
	while(bucket)
	{
		if(bucket->key == key)
			return bucket;

		bucket = bucket->overflow;
	}

	return NULL;
}

hashset_bucket_t *hashset_findBucket2(hashset_t *set, void *key)
{
	uint32_t hash = set->hashFunction(key);
	size_t index = hash % set->capacity;

	hashset_bucket_t *bucket = set->buckets[index];
	while(bucket)
	{
		if(bucket->key == key)
			return bucket;
		
		bucket = bucket->overflow;
	}

	bucket = halloc(NULL, sizeof(hashset_bucket_t));
	if(!bucket)
		return NULL;

	bucket->key = key;
	bucket->data = NULL;
	bucket->overflow = set->buckets[index];

	set->buckets[index] = bucket;

	return bucket;
}

void hashset_rehash(hashset_t *set, size_t capacity)
{
	hashset_bucket_t **buckets = set->buckets;
	size_t cCapacity = set->capacity;

	set->capacity = capacity;
	set->buckets = halloc(NULL, capacity * sizeof(hashset_bucket_t **));

	if(!set->buckets)
	{
		set->capacity = cCapacity;
		set->buckets = buckets;

		return;
	}

	memset(set->buckets, 0, capacity * sizeof(hashset_bucket_t **));

	for(size_t i=0; i<cCapacity; i++)
	{
		hashset_bucket_t *bucket = buckets[i];
		while(bucket)
		{
			hashset_bucket_t *next = bucket->overflow;

			if(bucket->key)
			{
				uint32_t hash = set->hashFunction(bucket->key);
				size_t index = hash % capacity;

				bucket->overflow = set->buckets[index];
				set->buckets[index] = bucket;
			}
			else
			{
				hfree(NULL, bucket);
			}

			bucket = next;
		}
	}

	hfree(NULL, buckets);
}

void hashset_expandIfNeeded(hashset_t *set)
{
	for(size_t i=0; i<41; i++)
	{
		if(set->capacity == hashet_capacity[i])
		{
			if(set->count >= hashset_maxCount[i])
			{
				size_t capacity = hashet_capacity[i + 1];
				hashset_rehash(set, capacity);
			}

			break;
		}
	}
}

void hashset_shrinkIfNeeded(hashset_t *set)
{
	for(size_t i=1; i<41; i++)
	{
		if(set->capacity == hashet_capacity[i])
		{
			if(set->count <= hashset_maxCount[i - 1])
			{
				size_t capacity = hashet_capacity[i - 1];
				hashset_rehash(set, capacity);
			}
		}
	}
}


void hashset_lock(hashset_t *set)
{
	spinlock_lock(&set->lock);
}
void hashset_unlock(hashset_t *set)
{
	spinlock_unlock(&set->lock);
}


void *hashset_dataForKey(hashset_t *set, void *key)
{
	hashset_bucket_t *bucket = hashset_findBucket1(set, key);
	return bucket ? bucket->data : NULL;
}

void hashset_removeDataForKey(hashset_t *set, void *key)
{
	hashset_bucket_t *bucket = hashset_findBucket1(set, key);
	if(bucket)
	{
		bucket->key  = NULL;
		bucket->data = NULL;

		set->count --;
		hashset_shrinkIfNeeded(set);
	}
}

void hashset_setDataForKey(hashset_t *set, void *data, void *key)
{
	if(!data)
	{
		hashset_removeDataForKey(set, key);
		return;
	}

	hashset_bucket_t *bucket = hashset_findBucket2(set, key);
	if(bucket->data == NULL)
	{
		bucket->data = data;

		set->count ++;
		hashset_expandIfNeeded(set);
	}
}

uint32_t hashset_count(hashset_t *set)
{
	return set->count;
}

// Hashing functions

uint32_t hash_pointer(void *key)
{
	return (uint32_t)key;
}

uint32_t hash_cstring(void *key)
{
	uint8_t *string = (uint8_t *)key;

	size_t length = strlen((const char *)string);
	uint32_t result = length;

	if(length <= 16) 
	{
		for(size_t i=0; i<length; i++) 
			result = result * 257 + string[i];
	} 
	else 
	{
		// Hash the first and last 8 bytes
		for(size_t i=0; i<8; i++) 
			result = result * 257 + string[i];
		
		for(size_t i=length - 8; i<length; i++) 
			result = result * 257 + string[i];
	}
	
	return (result << (length & 31));
}
