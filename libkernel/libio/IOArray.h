//
//  IOArray.h
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

#ifndef _IOARRAY_H_
#define _IOARRAY_H_

#include "IOObject.h"
#include "IOIterator.h"

class IOArray : public IOObject
{
public:
	virtual IOArray *init();
	virtual IOArray *initWithCapacity(size_t capacity);
	
	static IOArray *withCapacity(uint32_t capacity);

	void addObject(IOObject *object);
	void removeObject(IOObject *object);
	void removeObject(uint32_t index);
	void removeAllObjects();

	uint32_t indexOfObject(IOObject *object);
	bool containsObject(IOObject *object);

	IOObject *objectAtIndex(uint32_t index);

	void setChangeSize(uint32_t changeSize);
	uint32_t changeSize();

	virtual size_t count();
	virtual size_t capacity();

private:
	virtual void free();

	IOObject **_objects;
	uint32_t _count, _capacity;
	uint32_t _changeSize;

	IODeclareClass(IOArray)
};

#endif /* _IOARRAY_H_ */
