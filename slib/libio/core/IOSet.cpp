//
//  IOSet.cpp
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2015 by Sidney Just
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

#include "IOSet.h"
#include "IOHashtableInternal.h"

namespace IO
{
	IODefineMeta(Set, Object)
	
	class Set::Internal
	{
	public:
		struct Bucket
		{
			Bucket()
			{
				object = nullptr;
				next   = nullptr;
			}
			
			Bucket(const Bucket *other)
			{
				object = SafeRetain(other->object);
				next   = nullptr;
			}
			
			~Bucket()
			{
				SafeRelease(object);
			}
			
			
			bool WrapsLookup(const Object *lookup) const
			{
				return (object && lookup->IsEqual(object));
			}
			
			size_t GetHash() const
			{
				return object->GetHash();
			}
			
			Object *object;
			Bucket *next;
		};
		
		HashTableCore<Bucket> hashTable;
	};
	
	Set::Set()
	{}

	Set *Set::Init()
	{
		if(!Object::Init())
			return nullptr;

		_internals->hashTable.Initialize(0);

		return this;
	}

	Set *Set::InitWithCapacity(size_t capacity)
	{
		if(!Object::Init())
			return nullptr;

		_internals->hashTable.Initialize(capacity);

		return this;
	}

	void Set::Dealloc()
	{
		Object::Dealloc();
	}

	
	StrongRef<Array> Set::GetAllObjects() const
	{
		Array *array = Array::Alloc()->InitWithCapacity(_internals->hashTable.GetCount());
		
		for(size_t i = 0; i < _internals->hashTable._capacity; i++)
		{
			Internal::Bucket *bucket = _internals->hashTable._buckets[i];
			while(bucket)
			{
				if(bucket->object)
					array->AddObject(bucket->object);
				
				bucket = bucket->next;
			}
		}
		
		return IOTransferRef(array);
	}
	
	size_t Set::GetCount() const
	{
		return _internals->hashTable.GetCount();
	}
	
	
	void Set::AddObject(Object *object)
	{
		bool create;
		Internal::Bucket *bucket = _internals->hashTable.FindBucket(object, create);
		
		if(bucket && create)
		{
			bucket->object = object->Retain();
			_internals->hashTable.GrowIfPossible();
		}
	}
	
	void Set::RemoveObject(Object *key)
	{
		Internal::Bucket *bucket = _internals->hashTable.FindBucket(key);
		if(bucket)
		{
			SafeRelease(bucket->object);
			
			_internals->hashTable.ResignBucket(bucket);
			_internals->hashTable.CollapseIfPossible();
		}
	}
	
	void Set::RemoveAllObjects()
	{
		_internals->hashTable.RemoveAllBuckets();
	}
	
	bool Set::ContainsObject(Object *object) const
	{
		Internal::Bucket *bucket = _internals->hashTable.FindBucket(object);
		return (bucket != nullptr);
	}
	
	
	
	void Set::Enumerate(const Function<void (Object *, bool &)> &callback) const
	{
		bool stop = false;
		
		for(size_t i = 0; i < _internals->hashTable._capacity; i ++)
		{
			Internal::Bucket *bucket = _internals->hashTable._buckets[i];
			while(bucket)
			{
				if(bucket->object)
				{
					callback(bucket->object, stop);
					
					if(stop)
						return;
				}
				
				bucket = bucket->next;
			}
		}
	}
}
