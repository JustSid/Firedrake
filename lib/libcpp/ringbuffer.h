//
//  ringbuffer.h
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

#ifndef _STD_RINGBUFFER_H_
#define _STD_RINGBUFFER_H_

#include <libc/stdbool.h>
#include <libc/stddef.h>
#include <libc/assert.h>
#include "iterator.h"
#include "algorithm.h"
#include "type_traits.h"

#if __KERNEL
#include <kern/panic.h>
#endif

namespace std
{
	template<class T, size_t Size>
	class ringbuffer
	{
	public:
		ringbuffer() :
			_head(0),
			_tail(0),
			_size(0)
		{}


		void clear()
		{
			_head = _tail = 0;
			_size = 0;
		}

		T &pop()
		{
			if(_size > 0)
			{
				size_t head = _head;

				_head = (_head + 1) % Capacity;
				_size --;

				return _content[head];
			}

#if __KERNEL
			panic("pop() on empty rinbuffer");
#else
			throw "pop() on empty ringbuffer");
#endif
		}

		void push(T val)
		{
			_content[_tail] = val;
			increment();
		}


		size_t size() const
		{
			return _size;
		}


		T &front()
		{
			return _content[_head];
		}
		const T &front() const
		{
			return _content[_head];
		}

	private:
		void increment()
		{
			_tail = (_tail + 1) % Capacity;
			_size = std::min(_size + 1, static_cast<size_t>(Capacity));

			if(_size == Capacity)
				_head = _tail;
		}

		enum { Capacity = Size + 1 };

		T _content[Capacity];
		size_t _head;
		size_t _tail;
		size_t _size;
	};
}

#endif /* _STD_RINGBUFFER_H_ */
