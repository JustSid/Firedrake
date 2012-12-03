//
//  IODictionary.h
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

#ifndef _IODICTIONARY_H_
#define _IODICTIONARY_H_

#include "IOTypes.h"
#include "IOObject.h"
#include "IOIterator.h"

struct IODictionaryBucket
{
	IOObject *key;
	IOObject *object;

	struct IODictionaryBucket *next;
};

class IODictionaryIterator;

class IODictionary : public IOObject
{
friend class IODictionaryIterator;
public:
	virtual IODictionary *init();
	virtual IODictionary *initWithCapacity(size_t capacity);
	
	static IODictionary *dictionary();
	static IODictionary *withCapacity(size_t capacity);
	static IODictionary *withObjectsAndKeys(IOObject *object, IOObject *key, ...)  __attribute__((sentinel));

	virtual void setObjectForKey(IOObject *object, IOObject *key);
	virtual void removeObjectForKey(IOObject *key);

	virtual IOObject *objectForKey(IOObject *key);

	virtual size_t count();
	virtual size_t capacity();

	IOIterator *objectIterator();
	IOIterator *keyIterator();

protected:
	virtual void free();

	IODictionaryBucket *findBucket1(IOObject *key);
	IODictionaryBucket *findBucket2(IOObject *key);

	void rehash(size_t capacity);
	void expandIfNeeded();
	void shrinkIfNeeded();

	IODictionaryBucket **_buckets;

	size_t _capacity;
	size_t _count;

	IODeclareClass(IODictionary)
};

class IODatabase;
class __IOPointerDictionary : public IODictionary
{
friend class IODatabase;
public:
	static __IOPointerDictionary *withCapacity(size_t capacity);
	
	void setPointerForKey(void *ptr, IOObject *key);
	void removePointerForKey(IOObject *key);

	void *pointerForKey(IOObject *key);

	virtual void setObjectForKey(IOObject *object, IOObject *key);
	virtual void removeObjectForKey(IOObject *key);

	virtual IOObject *objectForKey(IOObject *key);

protected:
	virtual void free();

	IODeclareClass(__IOPointerDictionary)
};

#endif /* _IODICTIONARY_H_ */
