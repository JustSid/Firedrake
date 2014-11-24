//
//  IOHashtableInternal.h
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

#ifndef _IOHASHTABLEINTERNAL_H_
#define _IOHASHTABLEINTERNAL_H_

#include <libcpp/algorithm.h>
#include <libc/stdint.h>

#define kRIOashTablePrimitiveCount 64

namespace IO
{
	static const size_t kHashTableCapacity[kRIOashTablePrimitiveCount] =
	{
		3, 7, 13, 23, 41, 71, 127, 191, 251, 383, 631, 1087, 1723,
		2803, 4523, 7351, 11959, 19447, 31231, 50683, 81919, 132607,
		214519, 346607, 561109, 907759, 1468927, 2376191, 3845119,
		6221311, 10066421, 16287743, 26354171, 42641881, 68996069,
		111638519, 180634607, 292272623, 472907251
	};
	
	static const size_t kHashTableMaxCount[kRIOashTablePrimitiveCount] =
	{
		3, 6, 11, 19, 32, 52, 85, 118, 155, 237, 390, 672, 1065,
		1732, 2795, 4543, 7391, 12019, 19302, 31324, 50629, 81956,
		132580, 214215, 346784, 561026, 907847, 1468567, 2376414,
		3844982, 6221390, 10066379, 16287773, 26354132, 42641916,
		68996399, 111638327, 180634415, 292272755
	};
	
	
	template<class Bucket>
	class HashTableCore
	{
	public:
		HashTableCore()
		{}
		
		~HashTableCore()
		{
			for(size_t i = 0; i < _capacity; i ++)
			{
				Bucket *bucket = _buckets[i];
				while(bucket)
				{
					Bucket *next = bucket->next;
					delete bucket;
					
					bucket = next;
				}
			}
			
			delete [] _buckets;
		}
		
		void Initialize(size_t capacity)
		{
			size_t primitive = 0;
			
			for(size_t i = 0; i < kRIOashTablePrimitiveCount; i ++)
			{
				if(kHashTableCapacity[i] > capacity || i == kRIOashTablePrimitiveCount - 1)
				{
					primitive = i;
					break;
				}
			}
			
			_primitive = primitive;
			_capacity  = kHashTableCapacity[_primitive];
			_count     = 0;
			
			_buckets = new Bucket *[_capacity];
			std::fill(_buckets, _buckets + _capacity, nullptr);
		}
		
		void Initialize(const HashTableCore &other)
		{
			_primitive = other._primitive;
			_capacity  = other._capacity;
			_count     = other._count;
			
			_buckets = new Bucket *[_capacity];
			
			for(size_t i = 0; i < _capacity; i ++)
			{
				Bucket *temp = nullptr;
				Bucket *bucket = other._buckets[i];
				
				while(bucket)
				{
					if(bucket->object)
					{
						Bucket *copy = new Bucket(bucket);

						copy->next = temp;
						temp = copy;
					}
					
					bucket = bucket->next;
				}
				
				_buckets[i] = temp;
			}
		}
		
		
		
		
		Bucket *FindBucket(Object *object) const
		{
			size_t hash = object->GetHash();
			size_t index = hash % _capacity;
			
			Bucket *bucket = _buckets[index];
			while(bucket)
			{
				if(bucket->WrapsLookup(object))
					return bucket;
				
				bucket = bucket->next;
			}
			
			return bucket;
		}
		
		Bucket *FindBucket(Object *object, bool &created)
		{
			size_t hash = object->GetHash();
			size_t index = hash % _capacity;
			
			Bucket *bucket = _buckets[index];
			Bucket *empty  = nullptr;
			
			created = false;
			
			while(bucket)
			{
				if(bucket->WrapsLookup(object))
					return bucket;
				
				if(!bucket->object)
					empty = bucket;
				
				bucket = bucket->next;
			}
			
			created = true;
			
			if(empty)
			{
				_count ++;
				return empty;
			}
			
			bucket = new Bucket();
			bucket->next = _buckets[index];
			
			_buckets[index] = bucket;
			_count ++;
			
			return bucket;
		}
		
		bool ContainsObject(Object *object)
		{
			Bucket *bucket = FindBucket(object);
			return (bucket != nullptr);
		}
		
		
		void ResignBucket(Bucket *bucket)
		{
			bucket->object = nullptr;
			_count --;
		}
		
		void Rehash(size_t primitive)
		{
			size_t cCapacity = _capacity;
			Bucket **buckets = _buckets;
			
			_capacity = kHashTableCapacity[primitive];
			_buckets  = new Bucket *[_capacity];
			
			if(!_buckets)
			{
				_buckets  = buckets;
				_capacity = cCapacity;
				
				return;
			}
			
			_primitive = primitive;
			std::fill(_buckets, _buckets + _capacity, nullptr);
			
			for(size_t i = 0; i < cCapacity; i ++)
			{
				Bucket *bucket = buckets[i];
				while(bucket)
				{
					Bucket *next = bucket->next;
					
					if(bucket->object)
					{
						size_t hash = bucket->GetHash();
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
			
			delete [] buckets;
		}
	
		void RemoveAllBuckets()
		{
			for(size_t i = 0; i < _capacity; i ++)
			{
				Bucket *bucket = _buckets[i];
				while(bucket)
				{
					Bucket *next = bucket->next;
					delete bucket;
					
					bucket = next;
				}
			}
			
			delete [] _buckets;
			
			_count     = 0;
			_primitive = 1;
			_capacity  = kHashTableCapacity[_primitive];
			
			_buckets = new Bucket *[_capacity];
			std::fill(_buckets, _buckets + _capacity, nullptr);
		}
		
		void GrowIfPossible()
		{
			if(_count >= kHashTableMaxCount[_primitive] && _primitive < kRIOashTablePrimitiveCount)
				Rehash(_primitive + 1);
		}
		
		void CollapseIfPossible()
		{
			if(_primitive > 0 && _count <= kHashTableMaxCount[_primitive - 1])
				Rehash(_primitive - 1);
		}
		
		size_t GetCount() const
		{
			return _count;
		}
		
		
		Bucket **_buckets;
		size_t _count;
		size_t _capacity;
		
	private:
		size_t _primitive;
	};
}

#endif /* _IOHASHTABLEINTERNAL_H_ */
