//
//  IODictionary.cpp
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

#include <libkernel/base.h>
#include <libkernel/kalloc.h>

#include "IODictionary.h"
#include "IORuntime.h"

#ifdef super
#undef super
#endif
#define super IOObject

static uint32_t __IODictionaryCapacity[42] = 
{
	5, 11, 23, 41, 67, 113, 199, 317, 521, 839, 1361, 2207, 3571, 5779, 9349, 15121,
	24473, 39607, 64081, 103681, 167759, 271429, 439199, 710641, 1149857, 1860503, 3010349,
	4870843, 7881193, 12752029, 20633237, 33385273, 54018521, 87403763, 141422317, 228826121,
	370248451, 599074561, 969323023, 1568397599, 2537720629, 4106118251
};

static uint32_t __IODictionaryMaxCount[42] = 
{
	4, 8, 17, 29, 47, 76, 123, 199, 322, 521, 843, 1364, 2207, 3571, 5778, 9349,
	15127, 24476, 39603, 64079, 103682, 167761, 271443, 439204, 710647, 1149851, 1860498,
	3010349, 4870847, 7881196, 12752043, 20633239, 33385282, 54018521, 87403803, 141422324,
	228826127, 370248451, 599074578, 969323029, 1568397607, 2537720636
}; 


class IODictionaryIterator : public IOIterator
{
public:
	virtual IODictionaryIterator *initWithDictionary(IODictionary *dictionary, bool keyIterator)
	{
		if(IOIterator::init())
		{		
			_dictionary = dictionary;
			_keyIterator = keyIterator;
			_index  = 0;
			_bucket = 0;
		}

		return this;
	}

	virtual IOObject *nextObject()
	{
		while(!_bucket)
		{
			if(_index >= _dictionary->_capacity)
				return 0;

			_bucket = _dictionary->_buckets[_index ++];
		}

		IOObject *object = (_keyIterator) ? _bucket->key : _bucket->object;
		_bucket = _bucket->next;

		return object;
	}

private:
	IODictionary *_dictionary;
	IODictionaryBucket *_bucket;
	uint32_t _index;
	bool _keyIterator;

	IODeclareClass(IODictionaryIterator)
};

IORegisterClass(IODictionary, super);
IORegisterClass(IODictionaryIterator, IOIterator);

// Constructor

IODictionary *IODictionary::withCapacity(size_t capacity)
{
	IODictionary *dictionary = IODictionary::alloc()->initWithCapacity(capacity);
	return dictionary->autorelease();
}

IODictionary *IODictionary::withObjectsAndKeys(IOObject *object, IOObject *key, ...)
{
	IODictionary *dictionary = IODictionary::alloc()->init();

	va_list args;
	va_start(args, key);

	while(object && key)
	{
		dictionary->setObjectForKey(object, key);

		object = va_arg(args, IOObject *);

		if(object)
			key = va_arg(args, IOObject *);
	}

	va_end(args);
	return dictionary->autorelease();
}

IODictionary *IODictionary::init()
{
	if(super::init())
	{
		_capacity = __IODictionaryCapacity[0];
		_count = 0;

		_buckets = (IODictionaryBucket **)kalloc(_capacity * sizeof(IODictionaryBucket **));
		if(_buckets == 0)
		{
			release();
			return 0;
		}
	}

	return this;
}

IODictionary *IODictionary::initWithCapacity(size_t capacity)
{
	if(super::init())
	{
		for(int i=1; i<42; i++)
		{
			if(__IODictionaryCapacity[i] > capacity || i == 41)
			{
				_capacity = __IODictionaryCapacity[i - 1];
				break;
			}
		}

		_count = 0;

		_buckets = (IODictionaryBucket **)kalloc(_capacity * sizeof(IODictionaryBucket **));
		if(!_buckets)
		{
			release();
			return 0;
		}
	}

	return this;
}

void IODictionary::free()
{
	if(_buckets)
	{
		for(size_t i=0; i<_capacity; i++)
		{
			IODictionaryBucket *bucket = _buckets[i];
			while(bucket)
			{
				IODictionaryBucket *next = bucket->next;

				if(bucket->key)
				{
					bucket->key->release();
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

IODictionaryBucket *IODictionary::findBucket1(IOObject *key)
{
	hash_t hash = key->hash();
	size_t index = hash % _capacity;

	IODictionaryBucket *bucket = _buckets[index];
	while(bucket)
	{
		if(bucket->key && bucket->key->isEqual(key))
			return bucket;

		bucket = bucket->next;
	}

	return 0;
}

IODictionaryBucket *IODictionary::findBucket2(IOObject *key)
{
	hash_t hash = key->hash();
	size_t index = hash % _capacity;

	IODictionaryBucket *bucket = _buckets[index];
	while(bucket)
	{
		if(bucket->key && bucket->key->isEqual(key))
			return bucket;

		bucket = bucket->next;
	}

	bucket = new IODictionaryBucket;
	if(!bucket)
		return 0;

	bucket->key  = key;
	bucket->next = _buckets[index];

	_buckets[index] = bucket;
	return bucket;
}

// Resizing

void IODictionary::rehash(size_t capacity)
{
	IODictionaryBucket **buckets = _buckets;
	size_t cCapacity = _capacity;

	_capacity = capacity;
	_buckets  = (IODictionaryBucket **)kalloc(_capacity * sizeof(IODictionaryBucket **));

	if(!_buckets)
	{
		_capacity = cCapacity;
		_buckets = buckets;

		return;
	}

	// Re-add the old buckets
	for(size_t i=0; i<cCapacity; i++)
	{
		IODictionaryBucket *bucket = buckets[i];
		while(bucket)
		{
			IODictionaryBucket *next = bucket->next;

			if(bucket->key)
			{
				hash_t hash = bucket->key->hash();
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

void IODictionary::expandIfNeeded()
{
	for(size_t i=0; i<41; i++)
	{
		if(__IODictionaryCapacity[i] == _capacity)
		{
			if(_count >= __IODictionaryMaxCount[i])
			{
				size_t capacity = __IODictionaryCapacity[i + 1];
				rehash(capacity);
			}

			break;
		}
	}
}

void IODictionary::shrinkIfNeeded()
{
	for(size_t i=1; i<41; i++)
	{
		if(__IODictionaryCapacity[i] == _capacity)
		{
			if(_count <= __IODictionaryMaxCount[i - 1])
			{
				size_t capacity = __IODictionaryCapacity[i - 1];
				rehash(capacity);
			}
		}
	}
}

// Public API

void IODictionary::setObjectForKey(IOObject *object, IOObject *key)
{
	if(!object)
	{
		removeObjectForKey(key);
		return;
	}

	IODictionaryBucket *bucket = findBucket2(key);
	if(bucket)
	{
		if(bucket->object)
		{
			bucket->object->release();
			bucket->key->release();

			_count --;
		}

		bucket->object = object->retain();
		bucket->key    = key->retain();

		_count ++;
		expandIfNeeded();
	}
}

void IODictionary::removeObjectForKey(IOObject *key)
{
	IODictionaryBucket *bucket = findBucket1(key);
	if(bucket)
	{
		bucket->key->release();
		bucket->object->release();

		bucket->object = bucket->key = 0;
		_count --;

		shrinkIfNeeded();
	}
}

IOObject *IODictionary::objectForKey(IOObject *key)
{
	IODictionaryBucket *bucket = findBucket1(key);
	return (bucket != 0) ? bucket->object : 0;
}

size_t IODictionary::count()
{
	return _count;
}

size_t IODictionary::capacity()
{
	return _capacity;
}

IOIterator *IODictionary::objectIterator()
{
	IODictionaryIterator *iterator = IODictionaryIterator::alloc();
	iterator->initWithDictionary(this, false);

	return iterator->autorelease();
}

IOIterator *IODictionary::keyIterator()
{
	IODictionaryIterator *iterator = IODictionaryIterator::alloc();
	iterator->initWithDictionary(this, true);

	return iterator->autorelease();
}

// --------
// __IOPointerDictionary
// --------

IORegisterClass(__IOPointerDictionary, IODictionary);

void __IOPointerDictionary::free()
{
	if(_buckets)
	{
		for(size_t i=0; i<_capacity; i++)
		{
			IODictionaryBucket *bucket = _buckets[i];
			while(bucket)
			{
				IODictionaryBucket *next = bucket->next;

				if(bucket->key)
					bucket->key->release();

				delete bucket;
				bucket = next;
			}
		}

		kfree(_buckets);
	}

	super::free();
}

void __IOPointerDictionary::setPointerForKey(void *ptr, IOObject *key)
{
	if(!ptr)
	{
		removeObjectForKey(key);
		return;
	}

	IODictionaryBucket *bucket = findBucket2(key);
	if(bucket)
	{
		key->retain();

		if(bucket->object)
		{
			bucket->key->release();
			_count --;
		}

		bucket->object = (IOObject *)ptr;
		bucket->key    = key;

		_count ++;
		expandIfNeeded();
	}
}

void __IOPointerDictionary::removePointerForKey(IOObject *key)
{
	IODictionaryBucket *bucket = findBucket1(key);
	if(bucket)
	{
		bucket->key->release();

		bucket->object = 0;
		bucket->key = 0;

		_count --;
		shrinkIfNeeded();
	}
}

void *__IOPointerDictionary::pointerForKey(IOObject *key)
{
	IODictionaryBucket *bucket = findBucket1(key);
	return (bucket != 0) ? bucket->object : 0;
}


void __IOPointerDictionary::setObjectForKey(IOObject *UNUSED(object), IOObject *UNUSED(key))
{
	panic("__IOPointerDictionary::setObjectForKey() is illegal! Use setPointerForKey() instead!");
}

void __IOPointerDictionary::removeObjectForKey(IOObject *UNUSED(key))
{
	panic("__IOPointerDictionary::removeObjectForKey() is illegal! Use removePointerForKey() instead!");
}

IOObject *__IOPointerDictionary::objectForKey(IOObject *UNUSED(key))
{
	panic("__IOPointerDictionary::objectForKey() is illegal! Use pointerForKey() instead!");
}



