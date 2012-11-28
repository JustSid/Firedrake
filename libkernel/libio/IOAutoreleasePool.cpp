//
//  IOAutoreleasePool.cpp
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

#include <libkernel/string.h>
#include <libkernel/kalloc.h>

#include "IOAutoreleasePool.h"
#include "IOThread.h"
#include "IORuntime.h"

#ifdef super
#undef super
#endif
#define super IOObject

IORegisterClass(IOAutoreleasePool, super);

IOAutoreleasePool *IOAutoreleasePool::init()
{
	IOThread *owner = IOThread::currentThread();
	if(!owner)
	{
		release();
		return 0;
	}

	if(super::init())
	{
		_owner = owner;
		_count = 0;
		_size  = 5;
		_objects = (IOObject **)kalloc(_size * sizeof(IOObject *));

		if(_objects)
		{
			_previous = _owner->_topPool;
			_owner->_topPool = this;

			return this;
		}
	}

	release();
	return 0;
}

void IOAutoreleasePool::free()
{
	if(_owner)
	{
		if(this != _owner->_topPool)
			panic("IOAutoreleasePool::free() called, but IOAutoreleasePool is not the top most pool!");
	}

	while(_count)
	{
		IOObject **objects = _objects;
		size_t count = _count;

		_count = 0;
		_size  = 5;
		_objects = (IOObject **)kalloc(_size * sizeof(IOObject *));

		for(size_t i=0; i<count; i++)
		{
			IOObject *object = objects[i];
			object->release();
		}

		kfree(objects);
	}

	kfree(_objects);
	_owner->_topPool = _previous;

	super::free();
}



void IOAutoreleasePool::addObject(IOObject *object)
{
	if(_count >= _size)
	{
		size_t nsize = _size * 2;
		IOObject **nobjects = (IOObject **)kalloc(nsize * sizeof(IOObject *));

		if(!nobjects)
			panic("IOAutoreleasePool::addObject() failed, not enough memory!");

		memcpy(nobjects, _objects, _size * sizeof(IOObject *));
		kfree(_objects);

		_objects = nobjects;
		_size = nsize;
	}

	_objects[_count ++] = object;
}
