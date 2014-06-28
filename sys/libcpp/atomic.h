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
	template<class uint32_t>
	class atomic
	{};

	template<>
	class atomic<uint32_t>
	{
	public:
		atomic()
		{}

		atomic(uint32_t value) :
			_value(value)
		{}

		void store(uint32_t value)
		{
			memory_barrier();
			_value = value;
		}

		uint32_t load()
		{
			memory_barrier();
			return _value;
		}

		bool compare_exchange(uint32_t &expected, uint32_t desired)
		{
			return atomic_compare_swap_u32(expected, desired, &_value);
		}

		bool is_lock_free() const
		{
			return true;
		}

		uint32_t fetch_add(uint32_t value)
		{
			return atomic_add_u32(value, &_value);
		}

		uint32_t fetch_sub(uint32_t value)
		{
			return atomic_add_u32(-value, &_value);
		}

		uint32_t fetch_and(uint32_t arg)
		{
			return atomic_and_u32(arg, &_value);
		}

		uint32_t fetch_or(uint32_t arg)
		{
			return atomic_or_u32(arg, &_value);
		}

		uint32_t fetch_xor(uint32_t arg)
		{
			return atomic_xor_u32(arg, &_value);
		}

		uint32_t operator ++()
		{
			return fetch_add(1) + 1;
		}

		uint32_t operator ++(int)
		{
			return fetch_add(1);
		}

		uint32_t operator --()
		{
			return fetch_sub(1) - 1;
		}

		uint32_t operator --(int)
		{
			return fetch_sub(1);
		}

		uint32_t operator += (uint32_t value)
		{
			return fetch_add(value) + value;
		}

		uint32_t operator -= (uint32_t value)
		{
			return fetch_sub(value) - value;
		}

		uint32_t operator &= (uint32_t value)
		{
			return fetch_and(value) & value;
		}

		uint32_t operator |= (uint32_t value)
		{
			return fetch_or(value) | value;
		}

		uint32_t operator ^= (uint32_t value)
		{
			return fetch_xor(value) ^ value;
		}

	private:
		uint32_t _value;
	};

	template<>
	class atomic<int32_t>
	{
	public:
		atomic()
		{}

		atomic(int32_t value) :
			_value(value)
		{}

		void store(int32_t value)
		{
			memory_barrier();
			_value = value;
		}

		int32_t load()
		{
			memory_barrier();
			return _value;
		}

		bool compare_exchange(int32_t &expected, int32_t desired)
		{
			return atomic_compare_swap_i32(expected, desired, &_value);
		}

		bool is_lock_free() const
		{
			return true;
		}

		int32_t fetch_add(int32_t value)
		{
			return atomic_add_i32(value, &_value);
		}

		int32_t fetch_sub(int32_t value)
		{
			return atomic_add_i32(-value, &_value);
		}

		int32_t fetch_and(int32_t arg)
		{
			return atomic_and_i32(arg, &_value);
		}

		int32_t fetch_or(int32_t arg)
		{
			return atomic_or_i32(arg, &_value);
		}

		int32_t fetch_xor(int32_t arg)
		{
			return atomic_xor_i32(arg, &_value);
		}

		int32_t operator ++()
		{
			return fetch_add(1) + 1;
		}

		int32_t operator ++(int)
		{
			return fetch_add(1);
		}

		int32_t operator --()
		{
			return fetch_sub(1) - 1;
		}

		int32_t operator --(int)
		{
			return fetch_sub(1);
		}

		int32_t operator += (int32_t value)
		{
			return fetch_add(value) + value;
		}

		int32_t operator -= (int32_t value)
		{
			return fetch_sub(value) - value;
		}

		int32_t operator &= (int32_t value)
		{
			return fetch_and(value) & value;
		}

		int32_t operator |= (int32_t value)
		{
			return fetch_or(value) | value;
		}

		int32_t operator ^= (int32_t value)
		{
			return fetch_xor(value) ^ value;
		}

	private:
		int32_t _value;
	};
}

#endif /* _LIBCPP_ATOMIC_H_ */
