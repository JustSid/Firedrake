//
//  stdatomic.h
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

#ifndef _STDATOMIC_H_
#define _STDATOMIC_H_

#include "sys/cdefs.h"
#include "stdint.h"
#include "stdbool.h"

// More or less a copy and paste from the LLVM stdatomic header
// http://llvm.org/viewvc/llvm-project/cfe/trunk/lib/Headers/stdatomic.h?view=markup&pathrev=218957
// See also: http://en.cppreference.com/w/c/atomic
//
// Thread fences can be found in libc/sys/barriers.h

__BEGIN_DECLS

typedef enum
{
	memory_order_relaxed = __ATOMIC_RELAXED,
	memory_order_consume = __ATOMIC_CONSUME,
	memory_order_acquire = __ATOMIC_ACQUIRE,
	memory_order_release = __ATOMIC_RELEASE,
	memory_order_acq_rel = __ATOMIC_ACQ_REL,
	memory_order_seq_cst = __ATOMIC_SEQ_CST
} memory_order;

#define kill_dependency(y) (y)

#define ATOMIC_VAR_INIT(value) (value)
#define atomic_init __c11_atomic_int

#define atomic_store(object, desired) __c11_atomic_store(object, desired, __ATOMIC_SEQ_CST)
#define atomic_store_explicit __c11_atomic_store

#define atomic_load(object) __c11_atomic_load(object, __ATOMIC_SEQ_CST)
#define atomic_load_explicit __c11_atomic_load

#define atomic_exchange(object, desired) __c11_atomic_exchange(object, desired, __ATOMIC_SEQ_CST)
#define atomic_exchange_explicit __c11_atomic_exchange

#define atomic_compare_exchange_strong(object, expected, desired) __c11_atomic_compare_exchange_strong(object, expected, desired, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#define atomic_compare_exchange_strong_explicit __c11_atomic_compare_exchange_strong

#define atomic_compare_exchange_weak(object, expected, desired) __c11_atomic_compare_exchange_weak(object, expected, desired, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#define atomic_compare_exchange_weak_explicit __c11_atomic_compare_exchange_weak

#define atomic_fetch_add(object, operand) __c11_atomic_fetch_add(object, operand, __ATOMIC_SEQ_CST)
#define atomic_fetch_add_explicit __c11_atomic_fetch_add

#define atomic_fetch_sub(object, operand) __c11_atomic_fetch_sub(object, operand, __ATOMIC_SEQ_CST)
#define atomic_fetch_sub_explicit __c11_atomic_fetch_sub

#define atomic_fetch_or(object, operand) __c11_atomic_fetch_or(object, operand, __ATOMIC_SEQ_CST)
#define atomic_fetch_or_explicit __c11_atomic_fetch_or

#define atomic_fetch_xor(object, operand) __c11_atomic_fetch_xor(object, operand, __ATOMIC_SEQ_CST)
#define atomic_fetch_xor_explicit __c11_atomic_fetch_xor

#define atomic_fetch_and(object, operand) __c11_atomic_fetch_and(object, operand, __ATOMIC_SEQ_CST)
#define atomic_fetch_and_explicit __c11_atomic_fetch_and

#define atomic_thread_fence(order) __c11_atomic_thread_fence(order)

__END_DECLS

#endif /* _STDATOMIC_H_ */
