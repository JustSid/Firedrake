//
//  IOSet.h
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

#ifndef _IOSET_H_
#define _IOSET_H_

#include "IOTypes.h"
#include "IOObject.h"
#include "IOIterator.h"

struct IOSetBucket
{
	IOObject *object;

	struct IOSetBucket *next;
};

class IOSetIterator;

class IOSet : public IOObject
{
friend class IOSetIterator;
public:
	virtual IOSet *init();
	virtual IOSet *initWithCapacity(size_t capacity);

	IOSet *set();
	IOSet *withCapacity(size_t capacity);
	IOSet *withObjects(IOObject *object, ...) __attribute__((sentinel));

	void addObject(IOObject *object);
	void removeObject(IOObject *object);
	bool containsObject(IOObject *object);

	void removeAllObjects();

	IOIterator *objectIterator();

	virtual size_t count();
	virtual size_t capacity();

private:
	virtual void free();

	IOSetBucket *findBucket1(IOObject *key);
	IOSetBucket *findBucket2(IOObject *key);

	void rehash(size_t capacity);
	void expandIfNeeded();
	void shrinkIfNeeded();

	IOSetBucket **_buckets;

	size_t _capacity;
	size_t _count;

	IODeclareClass(IOSet)
};

#endif /* _IOSET_H_ */
