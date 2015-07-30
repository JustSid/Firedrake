//
//  IOArray.cpp
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2014 by Sidney Just
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

#include <libcpp/algorithm.h>
#include "IOArray.h"

namespace IO
{
	IODefineMeta(Array, Object)

	Array *Array::Init()
	{
		return InitWithCapacity(0);
	}

	Array *Array::InitWithCapacity(size_t size)
	{
		if(!Object::Init())
			return nullptr;

		_count = 0;
		_capacity = std::max(size, static_cast<size_t>(5));

		_data = new Object *[_capacity];

		return (_data != nullptr) ? this : nullptr;
	}

	Array *Array::InitWithArray(const Array *other)
	{
		if(!Object::Init())
			return nullptr;

		_count = 0;
		_capacity = other->_count;

		_data = new Object *[_capacity];

		if(_data)
		{
			_count = other->_count;

			for(size_t i = 0; i < _count; i ++)
			{
				_data[i] = other->_data[i]->Retain();
			}

			return this;
		}

		return nullptr;
	}

	void Array::Dealloc()
	{
		for(size_t i = 0; i < _count; i ++)
			_data[i]->Release();

		delete [] _data;

		Object::Dealloc();
	}


	void Array::AddObject(Object *object)
	{
		AssertCapacity(_count + 1);
		_data[_count ++] = object->Retain();
	}

	void Array::AddObjectsFromArray(const Array *other)
	{
		AssertCapacity(_count + other->_count);

		for(size_t i = 0; i < other->_count; i ++)
			_data[_count ++] = other->_data[i]->Retain();
	}

	void Array::RemoveObject(Object *object)
	{
		for(size_t i = 0; i < _count; i ++)
		{
			if(object->IsEqual(_data[i]))
			{
				RemoveObjectAtIndex(i);
				return;
			}
		}
	}

	void Array::RemoveObjectAtIndex(size_t index)
	{
		IOAssert(index < _count, "Index out of bounds");

		_data[index]->Release();

		for(size_t i = index; i < _count - 1; i ++)
			_data[i] = _data[i + 1];

		AssertCapacity(_count - 1);
		_count --;
	}

	void Array::RemoveAllObjects()
	{
		for(size_t i = 0; i < _count; i ++)
			_data[i]->Release();

		_count = 0;
	}

	size_t Array::GetIndexOfObject(Object *object) const
	{
		for(size_t i = 0; i < _count; i ++)
		{
			if(object->IsEqual(_data[i]))
				return i;
		}

		return kNotFound;
	}

	size_t Array::GetCount() const
	{
		return _count;
	}

	void Array::AssertCapacity(size_t required)
	{
		if(required >= _capacity)
		{
			size_t tsize = std::max(required, _capacity * 2);
			Object **tdata = new Object *[tsize];

			for(size_t i = 0; i < _count; i ++)
				tdata[i] = _data[i];

			delete [] _data;

			_capacity = tsize;
			_data     = tdata;

			return;
		}

		size_t tsize = _capacity >> 1;

		if(tsize >= required && tsize >= 5)
		{
			size_t toCopy = std::min(_count, required);
			Object **tdata = new Object *[tsize];

			for(size_t i = 0; i < toCopy; i ++)
				tdata[i] = _data[i];

			delete [] _data;

			_capacity = tsize;
			_data     = tdata;
		}
	}

	Object *Array::__GetObjectAtIndex(size_t index) const
	{
		IOAssert(index < _count, "Index out of bounds");
		return _data[index];
	}
	Object *Array::__GetFirstObject() const
	{
		IOAssert(_count > 0, "GetFirstObject on empty array");
		return _data[0];
	}
	Object *Array::__GetLastObject() const
	{
		IOAssert(_count > 0, "GetLastObject on empty array");
		return _data[_count - 1];
	}
}
