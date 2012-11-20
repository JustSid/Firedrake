//
//  IOSet.cpp
//  libio
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

#include <libkernel/kalloc.h>

#include "IOSet.h"

#ifdef super
#undef super
#endif
#define super IOObject

static uint32_t __IOSetCapacity[42] = 
{
	5, 11, 23, 41, 67, 113, 199, 317, 521, 839, 1361, 2207, 3571, 5779, 9349, 15121,
	24473, 39607, 64081, 103681, 167759, 271429, 439199, 710641, 1149857, 1860503, 3010349,
	4870843, 7881193, 12752029, 20633237, 33385273, 54018521, 87403763, 141422317, 228826121,
	370248451, 599074561, 969323023, 1568397599, 2537720629, 4106118251
};

static uint32_t __IOSetMaxCount[42] = 
{
	4, 8, 17, 29, 47, 76, 123, 199, 322, 521, 843, 1364, 2207, 3571, 5778, 9349,
	15127, 24476, 39603, 64079, 103682, 167761, 271443, 439204, 710647, 1149851, 1860498,
	3010349, 4870847, 7881196, 12752043, 20633239, 33385282, 54018521, 87403803, 141422324,
	228826127, 370248451, 599074578, 969323029, 1568397607, 2537720636
}; 


// Constructor

class IOSetIterator : public IOIterator
{
public:
	virtual IOSetIterator *initWithSet(IOSet *set)
	{
		if(IOIterator::init())
		{		
			_set = set;
			_index  = 0;
			_bucket = 0;
		}

		return this;
	}

	virtual IOObject *nextObject()
	{
		while(!_bucket)
		{
			if(_index >= _set->_capacity)
				return 0;

			_bucket = _set->_buckets[_index ++];
		}

		IOObject *object = _bucket->object;
		_bucket = _bucket->next;

		return object;
	}

private:
	IOSet *_set;
	IOSetBucket *_bucket;
	uint32_t _index;

	IODeclareClass(IOSetIterator)
};

IORegisterClass(IOSet, super);
IORegisterClass(IOSetIterator, IOIterator);

IOSet *IOSet::withCapacity(size_t capacity)
{
	IOSet *set = IOSet::alloc();
	if(!set->initWithCapacity(capacity))
	{
		set->release();
		return 0;
	}

	return set->autorelease();
}

IOSet *IOSet::init()
{
	if(super::init())
	{
		_capacity = __IOSetCapacity[0];
		_count = 0;

		_buckets = (IOSetBucket **)kalloc(_capacity * sizeof(IOSetBucket **));
		if(_buckets == 0)
		{
			release();
			return 0;
		}
	}

	return this;
}

IOSet *IOSet::initWithCapacity(size_t capacity)
{
	if(super::init())
	{
		for(int i=1; i<42; i++)
		{
			if(__IOSetCapacity[i] > capacity || i == 41)
			{
				_capacity = __IOSetCapacity[i - 1];
				break;
			}
		}

		_count = 0;
		_buckets = (IOSetBucket **)kalloc(_capacity * sizeof(IOSetBucket **));
		if(_buckets == 0)
		{
			release();
			return 0;
		}
	}

	return this;
}

void IOSet::free()
{
	if(_buckets)
	{
		for(size_t i=0; i<_capacity; i++)
		{
			IOSetBucket *bucket = _buckets[i];
			while(bucket)
			{
				IOSetBucket *next = bucket->next;

				if(bucket->object)
				{
					bucket->object->release();
				}

				delete bucket;
				bucket = next;
			}
		}

		kfree(_buckets);
	}

	super::free();
}

// Lookup

IOSetBucket *IOSet::findBucket1(IOObject *object)
{
	hash_t hash = object->hash();
	size_t index = hash % _capacity;

	IOSetBucket *bucket = _buckets[index];
	while(bucket)
	{
		if(bucket->object && bucket->object->isEqual(object))
			return bucket;

		bucket = bucket->next;
	}

	return 0;
}

IOSetBucket *IOSet::findBucket2(IOObject *object)
{
	hash_t hash = object->hash();
	size_t index = hash % _capacity;

	IOSetBucket *bucket = _buckets[index];
	while(bucket)
	{
		if(bucket->object && bucket->object->isEqual(object))
			return bucket;

		bucket = bucket->next;
	}

	bucket = new IOSetBucket;
	if(!bucket)
		return 0;

	bucket->object = object;
	bucket->next   = _buckets[index];

	_buckets[index] = bucket;

	return bucket;
}

// Resizing

void IOSet::rehash(size_t capacity)
{
	IOSetBucket **buckets = _buckets;
	size_t cCapacity = _capacity;

	_capacity = capacity;
	_buckets  = (IOSetBucket **)kalloc(_capacity * sizeof(IOSetBucket **));

	if(!_buckets)
	{
		_capacity = cCapacity;
		_buckets = buckets;

		return;
	}

	// Re-add the old buckets
	for(size_t i=0; i<cCapacity; i++)
	{
		IOSetBucket *bucket = buckets[i];
		while(bucket)
		{
			IOSetBucket *next = bucket->next;

			if(bucket->object)
			{
				hash_t hash = bucket->object->hash();
				size_t index = hash % _capacity;

				bucket->next = _buckets[index];
				_buckets[index] = bucket;
			}
			else
			{
				delete bucket;
			}

			bucket = next;
		}
	}

	kfree(buckets);
}

void IOSet::expandIfNeeded()
{
	for(size_t i=0; i<41; i++)
	{
		if(__IOSetCapacity[i] == _capacity)
		{
			if(_count >= __IOSetMaxCount[i])
			{
				size_t capacity = __IOSetCapacity[i + 1];
				rehash(capacity);
			}

			break;
		}
	}
}

void IOSet::shrinkIfNeeded()
{
	for(size_t i=1; i<41; i++)
	{
		if(__IOSetCapacity[i] == _capacity)
		{
			if(_count <= __IOSetMaxCount[i - 1])
			{
				size_t capacity = __IOSetCapacity[i - 1];
				rehash(capacity);
			}
		}
	}
}

// Public API

void IOSet::addObject(IOObject *object)
{
	if(!object)
	{
		removeObject(object);
		return;
	}

	IOSetBucket *bucket = findBucket2(object);
	if(bucket)
	{
		object->retain();

		if(bucket->object)
		{
			bucket->object->release();

			_count --;
		}

		bucket->object = object;
		_count ++;

		expandIfNeeded();
	}
}

void IOSet::removeObject(IOObject *object)
{
	IOSetBucket *bucket = findBucket1(object);
	if(bucket)
	{
		bucket->object->release();
		bucket->object = 0;

		_count --;
		shrinkIfNeeded();
	}
}

bool IOSet::containsObject(IOObject *object)
{
	IOSetBucket *bucket = findBucket1(object);
	return (bucket != 0);
}

void IOSet::removeAllObjects()
{
	for(size_t i=0; i<_capacity; i++)
	{
		IOSetBucket *bucket = _buckets[i];
		while(bucket)
		{
			IOSetBucket *next = bucket->next;

			if(bucket->object)
			{
				bucket->object->release();
			}

			delete bucket;
			bucket = next;
		}
	}

	kfree(_buckets);

	_capacity = __IOSetCapacity[0];
	_buckets = (IOSetBucket **)kalloc(_capacity * sizeof(IOSetBucket **));
	_count = 0;
}

IOIterator *IOSet::objectIterator()
{
	IOSetIterator *iterator = IOSetIterator::alloc()->initWithSet(this);
	return iterator->autorelease();
}

size_t IOSet::count()
{
	return _count;
}

size_t IOSet::capacity()
{
	return _capacity;
}
