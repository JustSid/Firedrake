//
//  IOArray.cpp
//  libio
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

#include <libkernel/string.h>
#include <libkernel/kalloc.h>

#include "IOArray.h"

#ifdef super
#undef super
#endif
#define super IOObject

// Constructor

IORegisterClass(IOArray, super);

IOArray *IOArray::init()
{
	if(super::init())
	{
		_capacity = _changeSize = 5;
		_count = 0;

		_objects = (IOObject **)kalloc(_capacity * sizeof(IOObject *));
		if(!_objects)
		{
			release();
			return 0;
		}
	}

	return this;
}

IOArray *IOArray::initWithCapacity(size_t capacity)
{
	if(super::init())
	{
		_capacity  = capacity;
		_count = 0;
		_changeSize = 5;

		_objects = (IOObject **)kalloc(_capacity * sizeof(IOObject *));
		if(!_objects)
			return 0;
	}

	return this;
}

void IOArray::free()
{
	for(uint32_t i=0; i<_count; i++)
	{
		IOObject *object = _objects[i];
		object->release();
	}

	kfree(_objects);
	super::free();
}

IOArray *IOArray::array()
{
	IOArray *array = IOArray::alloc()->init();
	return array->autorelease();
}

IOArray *IOArray::withCapacity(uint32_t capacity)
{
	IOArray *array = IOArray::alloc()->initWithCapacity(capacity);
	return array->autorelease();
}

IOArray *IOArray::withObjects(IOObject *first, ...)
{
	va_list args;
	size_t count = 1;

	// Count the arguments
	va_start(args, first);

	while(va_arg(args, IOObject *) != 0)
		count ++;

	va_end(args);

	// Create the array
	IOArray *array = IOArray::alloc()->initWithCapacity(count);
	array->addObject(first);

	va_start(args, first);

	IOObject *object;
	while((object = va_arg(args, IOObject *)))
	{
		array->addObject(object);
	}

	va_end(args);
	return array->autorelease();
}

// Public API

void IOArray::resize(size_t size)
{
	IOObject **temp = (IOObject **)kalloc(size * sizeof(IOObject *));
	if(!temp)
		panic("Failed to resize IOArray!");

	memcpy(temp, _objects, _count * sizeof(IOObject *));
	kfree(_objects);

	_objects  = temp;
	_capacity = _capacity + _changeSize;

}

void IOArray::addObject(IOObject *object)
{
	if(_count >= _capacity)
		resize((_capacity + _changeSize));

	object->retain();
	_objects[_count ++] = object;
}

void IOArray::insertObject(IOObject *object, uint32_t index)
{
	if(_count >= _capacity)
		resize((_capacity + _changeSize));

	for(uint32_t i=_count; i>index; i--)
	{
		_objects[i] = _objects[i - 1];
	}

	_objects[index] = object->retain();
	_count ++;
}

void IOArray::removeObject(IOObject *object)
{
	uint32_t index = indexOfObject(object);
	removeObject(index);
}

void IOArray::removeObject(uint32_t index)
{
	if(index == IONotFound || index >= _count)
		return;

	IOObject *object = _objects[index];
	object->release();

	memcpy(&_objects[index], &_objects[index + 1], _count - index);
	_count --;

	if((_capacity - (_changeSize * 2)) > _count)
	{
		IOObject **temp = (IOObject **)kalloc((_count + _changeSize) * sizeof(IOObject *));
		memcpy(temp, _objects, _count * sizeof(IOObject *));
		kfree(_objects);

		_objects = temp;
		_capacity = _count + _changeSize;
	}
}

void IOArray::removeAllObjects()
{
	for(uint32_t i=0; i<_count; i++)
	{
		IOObject *object = _objects[i];
		object->release();
	}

	_capacity = _changeSize;
	_count = 0;

	IOObject **temp = (IOObject **)kalloc(_capacity * sizeof(IOObject *));
	if(temp)
	{
		kfree(_objects);
		_objects = temp;
	}
}


uint32_t IOArray::indexOfObject(IOObject *object)
{
	for(uint32_t i=0; i<_count; i++)
	{
		IOObject *tobject = _objects[i];

		if(tobject->isEqual(object))
			return i;
	}

	return IONotFound;
}

bool IOArray::containsObject(IOObject *object)
{
	for(uint32_t i=0; i<_count; i++)
	{
		IOObject *tobject = _objects[i];
		
		if(tobject->isEqual(object))
			return true;
	}

	return false;
}


IOObject *IOArray::objectAtIndex(uint32_t index)
{
	if(index != IONotFound && index < _count)
	{
		return _objects[index];
	}

	return 0;
}

void IOArray::setChangeSize(uint32_t tchangeSize)
{
	if(tchangeSize != _changeSize && tchangeSize >= 1)
	{
		_changeSize = tchangeSize;
	}
}

uint32_t IOArray::changeSize()
{
	return _changeSize;
}

size_t IOArray::count()
{
	return _count;
}

size_t IOArray::capacity()
{
	return _capacity;
}
