//
//  hash_map.h
//  Firedrake
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

#ifndef _HASH_MAP_H_
#define _HASH_MAP_H_

#include <libc/stdbool.h>
#include <libc/stddef.h>
#include <libc/string.h>
#include "functional.h"
#include "iterator.h"

namespace std
{
	namespace __hash
	{
		extern size_t capacity_primitive[];
		extern size_t count_primitive[];
	}

	template<class Key, class Val, class Hash = std::hash<Key>, class EqualTo = std::equal_to<Key>>
	class hash_map
	{
	private:
		struct bucket
		{
			Key key;
			Val value;
			bool valid;
			bucket *next;
		};

	public:
		class iterator : public std::iterator<std::input_iterator_tag, Val>
		{
		public:
			friend class hash_map;

			typename iterator::reference operator* () const
			{
				return _bucket->value;
			}
			typename iterator::pointer operator-> () const
			{
				return &_bucket->value;
			}

			bool operator!= (const iterator& other)
			{
				return (_bucket != other._bucket);
			}

			bool operator== (const iterator& other)
			{
				return (_bucket == other._bucket);
			}


		private:
			iterator(bucket *tbucket) :
				_bucket(tbucket)
			{}

			bucket *_bucket;
		};

		friend class iterator;

		hash_map() :
			_count(0),
			_capacity(0),
			_primitive(0)
		{
			_capacity = __hash::capacity_primitive[_primitive];
			_buckets  = new bucket *[_capacity];

			memset(_buckets, 0, _capacity * (sizeof(bucket **)));
		}

		~hash_map()
		{
			for(size_t i = 0; i < _capacity; i ++)
			{
				bucket *tbucket = _buckets[i];
				while(tbucket)
				{
					bucket *next = tbucket->next;
					delete tbucket;

					tbucket = next;
				}
			}
		}

		iterator find(const Key& key)
		{
			bucket *tbucket = find_bucket(key, true);
			return tbucket ? iterator(tbucket) : end();
		}

		iterator end() const
		{
			return iterator(nullptr);
		}

		void insert(const Key& key, const Val& value)
		{
			bucket *tbucket = find_bucket(key, false);
			tbucket->value  = value;

			grow_buckets();
		}


		void erase(const Key& key)
		{
			bucket *tbucket = find_bucket(key, true);
			if(tbucket)
			{
				tbucket->valid = false;
				_count --;

				collapse_buckets();
			}
		}
		void erase(const iterator& it)
		{
			if(it->_bucket)
			{
				it->_bucket->valid = false;
				_count --;

				collapse_buckets();
			}
		}

		size_t get_count() const
		{
			return _count;
		}
		size_t get_capacity() const
		{
			return _capacity;
		}

	private:
		void grow_buckets()
		{
			if(_count >= __hash::count_primitive[_primitive] && _primitive < 39)
				rehash(_primitive + 1);
		}

		void collapse_buckets()
		{
			if(_primitive > 0 && _count <= __hash::count_primitive[_primitive - 1])
				rehash(_primitive - 1);
		}

		void rehash(size_t primitive)
		{
			size_t cCapacity = _capacity;
			bucket **buckets = _buckets;
			
			_primitive = primitive;
			_capacity  = __hash::capacity_primitive[_primitive];
			_buckets   = new bucket *[_capacity];
			
			memset(_buckets, 0, _capacity * sizeof(bucket **));
			
			for(size_t i = 0; i < cCapacity; i ++)
			{
				bucket *tbucket = buckets[i];
				while(tbucket)
				{
					bucket *next = tbucket->next;
					
					if(tbucket->valid)
					{
						size_t index = _hasher(tbucket->key) % _capacity;
						
						tbucket->next   = _buckets[index];
						_buckets[index] = tbucket;
					}
					else
						delete tbucket;
					
					tbucket = next;
				}
			}
			
			delete [] buckets;
		}

		bucket *find_bucket(const Key& key, bool lookup)
		{
			size_t index = _hasher(key) % _capacity;
			bucket *tbucket = _buckets[index];

			if(lookup)
			{
				while(tbucket)
				{
					if(tbucket->valid && _comparator(tbucket->key, key))
						return tbucket;
					
					tbucket = tbucket->next;
				}
			}
			else
			{
				bucket *temp = nullptr;

				while(tbucket)
				{
					if(tbucket->valid && _comparator(tbucket->key, key))
						return tbucket;

					if(!tbucket->valid)
						temp = tbucket;
					
					tbucket = tbucket->next;
				}

				if(temp)
				{
					_count ++;
					return temp;
				}
			}

			tbucket = new bucket();
			tbucket->key = key;
			tbucket->valid = true;

			tbucket->next = _buckets[index];
			_buckets[index] = tbucket;

			_count ++;
			return tbucket;
		}

		Hash _hasher;
		EqualTo _comparator;

		bucket **_buckets;
		size_t _count;
		size_t _capacity;
		size_t _primitive;
	};
}

#endif /* _HASH_MAP_H_ */
