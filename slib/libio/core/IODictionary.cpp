//
//  IODictionary.cpp
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

#include "IODictionary.h"
#include "IOHashtableInternal.h"

namespace IO
{
	__IODefineIOCoreMeta(Dictionary, Object)
	
	class Dictionary::Internal
	{
	public:
		struct Bucket
		{
			Bucket()
			{
				key    = nullptr;
				object = nullptr;
				next   = nullptr;
			}
			
			Bucket(const Bucket *other)
			{
				key    = SafeRetain(other->key);
				object = SafeRetain(other->object);
				
				next = nullptr;
			}
			
			~Bucket()
			{
				SafeRelease(key);
				SafeRelease(object);
			}
			
			bool WrapsLookup(Object *lookup)
			{
				return (key && key->IsEqual(lookup));
			}
			
			size_t GetHash() const
			{
				return key->GetHash();
			}
			
			Object *key;
			Object *object;
			
			Bucket *next;
		};
		
		HashTableCore<Bucket> hashTable;
	};

	Dictionary::Dictionary()
	{}



	Dictionary *Dictionary::Init()
	{
		return InitWithCapacity(0);
	}

	Dictionary *Dictionary::InitWithCapacity(size_t capacity)
	{
		if(!Object::Init())
			return nullptr;

		_internals->hashTable.Initialize(capacity);

		return this;
	}

	void Dictionary::Dealloc()
	{
		Object::Dealloc();
	}
	
	
	size_t Dictionary::GetCount() const
	{
		return _internals->hashTable.GetCount();
	}
	
	
	Object *Dictionary::__GetPrimitiveObjectForKey(Object *key) const
	{
		Internal::Bucket *bucket = _internals->hashTable.FindBucket(key);
		return bucket ? bucket->object : nullptr;
	}
	
	void Dictionary::SetObjectForKey(Object *object, Object *key)
	{
		bool created;
		Internal::Bucket *bucket = _internals->hashTable.FindBucket(key, created);
		
		if(bucket)
		{
			SafeRelease(bucket->object);
			SafeRelease(bucket->key);
			
			bucket->key    = key->Retain();
			bucket->object = object->Retain();
			
			if(created)
				_internals->hashTable.GrowIfPossible();
		}
	}
	
	void Dictionary::RemoveObjectForKey(Object *key)
	{
		Internal::Bucket *bucket = _internals->hashTable.FindBucket(key);
		
		if(bucket)
		{
			SafeRelease(bucket->key);
			SafeRelease(bucket->object);
			
			_internals->hashTable.ResignBucket(bucket);
			_internals->hashTable.CollapseIfPossible();
		}
	}
	
	void Dictionary::RemoveAllObjects()
	{
		_internals->hashTable.RemoveAllBuckets();
	}
	
	void Dictionary::__Enumerate(const Function<void (Object *, Object *, bool &)> &callback) const
	{
		bool stop = false;
		
		for(size_t i = 0; i < _internals->hashTable._capacity; i ++)
		{
			Internal::Bucket *bucket = _internals->hashTable._buckets[i];
			while(bucket)
			{
				if(bucket->key)
				{
					callback(bucket->object, bucket->key, stop);
					
					if(stop)
						return;
				}
				
				bucket = bucket->next;
			}
		}
	}
}
