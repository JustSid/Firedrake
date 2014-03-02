//
//  atomic.h
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

#ifndef _LIBCPP_ATOMIC_H_
#define _LIBCPP_ATOMIC_H_

#include <machine/atomic.h>

namespace std
{
	template<class T>
	class atomic
	{
	public:
		atomic()
		{}

		atomic(T value) :
			_value(value)
		{}

		void store(T value)
		{
			memory_barrier();
			_value = value;
		}

		T load()
		{
			memory_barrier();
			return _value;
		}

		bool compare_exchange(T& expected, T desired)
		{
			return atomic_compare_swap(expected, desired, &_value);
		}

		bool is_lock_free() const
		{
			return true;
		}

		T fetch_add(T value)
		{
			return atomic_add(value, &_value);
		}

		T fetch_sub(T value)
		{
			return atomic_add(-value, &_value);
		}

		T fetch_and(T arg)
		{
			return atomic_and(arg, &_value);
		}

		T fetch_or(T arg)
		{
			return atomic_or(arg, &_value);
		}

		T fetch_xor(T arg)
		{
			return atomic_xor(arg, &_value);
		}

		T operator ++()
		{
			return fetch_add(1) + 1;
		}

		T operator ++(int)
		{
			return fetch_add(1);
		}

		T operator --()
		{
			return fetch_sub(1) - 1;
		}

		T operator --(int)
		{
			return fetch_sub(1);
		}

		T operator += (T value)
		{
			return fetch_add(value) + value;
		}

		T operator -= (T value)
		{
			return fetch_sub(value) - value;
		}

		T operator &= (T value)
		{
			return fetch_and(value) & value;
		}

		T operator |= (T value)
		{
			return fetch_or(value) | value;
		}

		T operator ^= (T value)
		{
			return fetch_xor(value) ^ value;
		}

	private:
		T _value;
	};
}

#endif /* _LIBCPP_ATOMIC_H_ */
