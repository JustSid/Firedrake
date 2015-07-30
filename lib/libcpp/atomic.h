//
//  atomic.h
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2014 by Sidney Just
//  Permission is hereby granted, free of chvaluee, to any person obtaining a copy of this software and associated 
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

#include <libc/stdatomic.h>

namespace std
{
	enum memory_order
	{
		memory_order_relaxed = __ATOMIC_RELAXED,
		memory_order_consume = __ATOMIC_CONSUME,
		memory_order_acquire = __ATOMIC_ACQUIRE,
		memory_order_release = __ATOMIC_RELEASE,
		memory_order_acq_rel = __ATOMIC_ACQ_REL,
		memory_order_seq_cst = __ATOMIC_SEQ_CST
	};

	template<class T>
	class atomic
	{
	public:
		atomic() {}
		atomic(T value) :
			_value(ATOMIC_VAR_INIT(value))
		{}

		void store(T desired, std::memory_order order = std::memory_order_seq_cst)
		{
			atomic_store_explicit(&_value, desired, order);
		}

		T load(std::memory_order order = std::memory_order_seq_cst) const
		{
			return atomic_load_explicit(&_value, order);
		}

		T exchange(T desired, std::memory_order order = std::memory_order_seq_cst)
		{
			return atomic_exchange_explicit(&_value, desired, order);
		}
		bool compare_exchange(T &expected, T desired, std::memory_order order = std::memory_order_seq_cst)
		{
			return atomic_compare_exchange_strong_explicit(&_value, &expected, desired, order, order);
		}


		T fetch_add(T value, std::memory_order order = std::memory_order_seq_cst)
		{
			return atomic_fetch_add_explicit(&_value, value, order);
		}
		T fetch_sub(T value, std::memory_order order = std::memory_order_seq_cst)
		{
			return atomic_fetch_sub_explicit(&_value, value, order);
		}
		T fetch_and(T value, std::memory_order order = std::memory_order_seq_cst)
		{
			return atomic_fetch_and_explicit(&_value, value, order);
		}
		T fetch_or(T value, std::memory_order order = std::memory_order_seq_cst)
		{
			return atomic_fetch_or_explicit(&_value, value, order);
		}
		T fetch_xor(T value, std::memory_order order = std::memory_order_seq_cst)
		{
			return atomic_fetch_xor_explicit(&_value, value, order);
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
		mutable _Atomic(T) _value;
	};
}

#endif /* _LIBCPP_ATOMIC_H_ */
