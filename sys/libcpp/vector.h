//
//  vector.h
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

#include <libc/stdbool.h>
#include <libc/stddef.h>
#include <kern/kalloc.h>
#include "iterator.h"
#include "algorithm.h"
#include "type_traits.h"

#ifndef _VECTOR_H_
#define _VECTOR_H_

namespace std
{
	template<class T>
	class vector
	{
	public:
		typedef T value_type;
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;
		typedef value_type& reference;
		typedef const value_type& const_reference;
		typedef value_type* pointer;
		typedef const value_type* const_pointer;

		class iterator : public std::iterator<std::random_access_iterator_tag, T>
		{
		public:
			friend class vector;

			iterator(const iterator &it) :
				_vector(it._vector),
				_index(it._index)
			{}

			typename iterator::reference operator* () const
			{
				return _vector->_data[_index];
			}
			typename iterator::pointer operator-> () const
			{
				return &_vector->_data[_index];
			}

			bool operator!= (const iterator &other) const
			{
				return !(*this == other);
			}
			bool operator== (const iterator &other) const
			{
				return (_index == other._index || (is_over_end() && other.is_over_end()));
			}

			iterator &operator++ ()
			{
				_index ++;
				return *this;
			}
			iterator operator++ (int)
			{
				iterator it(*this);
				_index ++;
				return it;
			}

			iterator &operator-- ()
			{
				_index --;
				return *this;
			}
			iterator operator-- (int)
			{
				iterator it(*this);
				_index --;
				return it;
			}

			iterator &operator+= (difference_type offset)
			{
				_index += offset;
				return *this;
			}
			iterator &operator-= (difference_type offset)
			{
				_index -= offset;
				return *this;
			}

			iterator operator+ (difference_type offset) const
			{
				iterator it(*this);
				it._index += offset;
				return it;
			}
			iterator operator- (difference_type offset) const
			{
				iterator it(*this);
				it._index -= offset;
				return it;
			}

			operator size_type()
			{
				return _index;
			}

		private:
			iterator(const vector *vector, size_type index) :
				_vector(vector),
				_index(index)
			{}

			bool is_over_end() const
			{
				return (_index >= _vector->_size);
			}

			const vector *_vector;
			size_type _index;
		};

		friend class iterator;

		vector() :
			_size(0),
			_capacity(5)
		{
			_data = reinterpret_cast<T *>(kalloc(_capacity * sizeof(T)));
		}
		~vector()
		{
			clear();
			kfree(_data);
		}

		reference at(size_type index) { return _data[index]; }
		const_reference at(size_type index) const { return _data[index]; }

		reference front() { return _data[0]; }
		const_reference front() const { return _data[0]; }

		reference back() { return _data[_size - 1]; }
		const_reference back() const { return _data[_size - 1]; }

		pointer data() { return _data; }
		const_pointer data() const { return _data; }

		iterator begin() { return iterator(this, 0); }
		iterator end() { return iterator(this, _size); }

		bool empty() const { return (_size == 0); }
		size_type size() const { return _size; }
		size_type capacity() const { return _capacity; }

		void clear()
		{
			for(size_type i = 0; i < _size; i ++)
				(_data + i)->~T();

			_size = 0;
		}

		void push_back(const_reference value)
		{
			if(_size >= _capacity)
				update_capacity(_capacity * 2);
			
			_data[_size ++] = value;
		}
		void push_back(T &&value)
		{
			if(_size >= _capacity)
				update_capacity(_capacity * 2);
			
			_data[_size ++] = std::move(value);
		}

		void erase(size_type index)
		{
			std::copy(_data + index + 1, _data + _size, _data + index);
			(_data + (_size - 1))->~T();

			if((-- _size) < _capacity / 2)
			{
				size_type nCapacity = std::max(_capacity, static_cast<size_type>(5));
				update_capacity(nCapacity);
			}
		}

	private:
		void update_capacity(size_type size)
		{
			if(size == _capacity)
				return;

			T *data = reinterpret_cast<T *>(kalloc(size * sizeof(T)));
			size_type toCopy = std::min(size, _size);

			std::copy(_data, _data + toCopy, data);

			for(size_type i = 0; i < _size; i ++)
				(_data + i)->~T();
			kfree(_data);

			_data     = data;
			_capacity = size;
		}

		T *_data;
		size_type _size;
		size_type _capacity;
	};
}

#endif

