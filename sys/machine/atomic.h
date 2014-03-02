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

#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#include <prefix.h>
#include <libc/stdint.h>
#include <libc/stdbool.h>

BEGIN_EXTERNC

bool atomic_compare_swap(int32_t oldValue, int32_t newValue, int32_t *address);
int32_t atomic_add(int32_t amount, int32_t *address);

static inline int32_t atomic_increment(int32_t *address)
{
	return atomic_add(1, address);
}

static inline int32_t atomic_decrement(int32_t *address)
{
	return atomic_add(-1, address);
}


static inline int32_t atomic_bitwise(int32_t andMask, int32_t orMask, int32_t xorMask, int32_t *address)
{
	int32_t oldValue;
	int32_t newValue;

	do {
		oldValue = *address;
		newValue = ((oldValue & andMask) | orMask) ^ xorMask;
	} while(!atomic_compare_swap(oldValue, newValue, address));

	return oldValue;
}

static inline int32_t atomic_and(int32_t mask, int32_t *address)
{
	return atomic_bitwise(mask, 0, 0, address);
}

static inline int32_t atomic_or(int32_t mask, int32_t *address)
{
	return atomic_bitwise(-1, mask, 0, address);
}

static inline int32_t atomic_xor(int32_t mask, int32_t *address)
{
	return atomic_bitwise(-1, 0, mask, address);
}

static inline void memory_barrier()
{
	// See: http://stackoverflow.com/questions/2599238
	// Basically: The lock prefix is faster and acts as a memory barrier just as well as mfence
	__asm__ volatile("lock orl $0, (%esp)");
}

END_EXTERNC

#endif /* _ATOMIC_H_ */
