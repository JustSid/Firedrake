//
//  IOObject.cpp
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

#include "IOObject.h"

namespace IO
{
	void *__kRNObjectMetaClass = nullptr;

	class ObjectMetaType : public MetaClass
	{
	public:
		ObjectMetaType() :
			MetaClass(nullptr, __PRETTY_FUNCTION__)
		{}
	};


	Object::Object() :
		_references(1)
	{}

	Object::~Object()
	{}

	Object *Object::Init()
	{
		return this;
	}

	void Object::Dealloc()
	{}


	Object *Object::Retain()
	{
		_references ++;
		return this;
	}

	Object *Object::Release()
	{
		if((-- _references) == 0)
		{
			Dealloc();
			delete this;

			return nullptr;
		}

		return this;
	}


	MetaClass *Object::GetClass() const
	{
		return Object::GetMetaClass();
	}
	MetaClass *Object::GetMetaClass()
	{
		if(!__kRNObjectMetaClass)
			__kRNObjectMetaClass = new ObjectMetaType();
		
		return reinterpret_cast<ObjectMetaType *>(__kRNObjectMetaClass);
	}


	size_t Object::GetHash() const
	{
		size_t hash = reinterpret_cast<size_t>(this);
				
		hash = ~hash + (hash << 15);
		hash = hash ^ (hash >> 12);
		hash = hash + (hash << 2);
		hash = hash ^ (hash >> 4);
		hash = hash * 2057;
		hash = hash ^ (hash >> 16);
		
		return hash;
	}

	bool Object::IsEqual(Object *other) const
	{
		return (this == other);
	}

	bool Object::IsKindOfClass(MetaClass *other) const
	{
		return GetClass()->InheritsFromClass(other);
	}
}
