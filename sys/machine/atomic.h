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

// -------------
// MARK: -
// MARK: 32 Bit
// -------------

__BEGIN_DECLS

bool atomic_compare_swap_i32(int32_t oldValue, int32_t newValue, int32_t *address);
bool atomic_compare_swap_u32(uint32_t oldValue, uint32_t newValue, uint32_t *address);
int32_t atomic_add_i32(int32_t amount, int32_t *address);
uint32_t atomic_add_u32(uint32_t amount, uint32_t *address);

__END_DECLS

// int32_t
static inline int32_t atomic_increment_i32(int32_t *address)
{
	return atomic_add_i32(1, address);
}
static inline int32_t atomic_decrement_i32(int32_t *address)
{
	return atomic_add_i32(-1, address);
}


static inline int32_t atomic_bitwise_i32(int32_t andMask, int32_t orMask, int32_t xorMask, int32_t *address)
{
	int32_t oldValue;
	int32_t newValue;

	do {
		oldValue = *address;
		newValue = ((oldValue & andMask) | orMask) ^ xorMask;
	} while(!atomic_compare_swap_i32(oldValue, newValue, address));

	return oldValue;
}
static inline int32_t atomic_and_i32(int32_t mask, int32_t *address)
{
	return atomic_bitwise_i32(mask, 0, 0, address);
}
static inline int32_t atomic_or_i32(int32_t mask, int32_t *address)
{
	return atomic_bitwise_i32(-1, mask, 0, address);
}
static inline int32_t atomic_xor_i32(int32_t mask, int32_t *address)
{
	return atomic_bitwise_i32(-1, 0, mask, address);
}

// uint32_t
static inline uint32_t atomic_increment_i32(uint32_t *address)
{
	return atomic_add_u32(1, address);
}
static inline uint32_t atomic_decrement_i32(uint32_t *address)
{
	return atomic_add_u32(-1, address);
}


static inline uint32_t atomic_bitwise_u32(uint32_t andMask, uint32_t orMask, uint32_t xorMask, uint32_t *address)
{
	uint32_t oldValue;
	uint32_t newValue;

	do {
		oldValue = *address;
		newValue = ((oldValue & andMask) | orMask) ^ xorMask;
	} while(!atomic_compare_swap_u32(oldValue, newValue, address));

	return oldValue;
}
static inline uint32_t atomic_and_u32(uint32_t mask, uint32_t *address)
{
	return atomic_bitwise_u32(mask, 0, 0, address);
}
static inline uint32_t atomic_or_u32(uint32_t mask, uint32_t *address)
{
	return atomic_bitwise_u32(-1, mask, 0, address);
}
static inline uint32_t atomic_xor_u32(uint32_t mask, uint32_t *address)
{
	return atomic_bitwise_u32(-1, 0, mask, address);
}

// -------------
// MARK: -
// MARK: 64 Bit
// -------------

__BEGIN_DECLS

bool atomic_compare_swap_i64(int64_t oldValue, int64_t newValue, int64_t *address);
bool atomic_compare_swap_u64(uint64_t oldValue, uint64_t newValue, uint64_t *address);
int64_t atomic_add_i64(int64_t amount, int64_t *address);
uint64_t atomic_add_u64(uint64_t amount, uint64_t *address);

__END_DECLS

// int64_t
static inline int64_t atomic_increment_i64(int64_t *address)
{
	return atomic_add_i64(1, address);
}
static inline int64_t atomic_decrement_i64(int64_t *address)
{
	return atomic_add_i64(-1, address);
}


static inline int64_t atomic_bitwise_i64(int64_t andMask, int64_t orMask, int64_t xorMask, int64_t *address)
{
	int64_t oldValue;
	int64_t newValue;

	do {
		oldValue = *address;
		newValue = ((oldValue & andMask) | orMask) ^ xorMask;
	} while(!atomic_compare_swap_i64(oldValue, newValue, address));

	return oldValue;
}
static inline int64_t atomic_and_i64(int64_t mask, int64_t *address)
{
	return atomic_bitwise_i64(mask, 0, 0, address);
}
static inline int64_t atomic_or_i64(int64_t mask, int64_t *address)
{
	return atomic_bitwise_i64(-1, mask, 0, address);
}
static inline int64_t atomic_xor_i64(int64_t mask, int64_t *address)
{
	return atomic_bitwise_i64(-1, 0, mask, address);
}

// uint64_t
static inline uint64_t atomic_increment_i64(uint64_t *address)
{
	return atomic_add_u64(1, address);
}
static inline uint64_t atomic_decrement_i64(uint64_t *address)
{
	return atomic_add_u64(-1, address);
}


static inline uint64_t atomic_bitwise_u64(uint64_t andMask, uint64_t orMask, uint64_t xorMask, uint64_t *address)
{
	uint64_t oldValue;
	uint64_t newValue;

	do {
		oldValue = *address;
		newValue = ((oldValue & andMask) | orMask) ^ xorMask;
	} while(!atomic_compare_swap_u64(oldValue, newValue, address));

	return oldValue;
}
static inline uint64_t atomic_and_u64(uint64_t mask, uint64_t *address)
{
	return atomic_bitwise_u64(mask, 0, 0, address);
}
static inline uint64_t atomic_or_u64(uint64_t mask, uint64_t *address)
{
	return atomic_bitwise_u64(-1, mask, 0, address);
}
static inline uint64_t atomic_xor_u64(uint64_t mask, uint64_t *address)
{
	return atomic_bitwise_u64(-1, 0, mask, address);
}

// -------------
// MARK: -
// MARK: Misc
// -------------

static inline void memory_fence()
{
	__asm__ volatile("" ::: "memory");
}

static inline void memory_barrier()
{
	// See: http://stackoverflow.com/questions/2599238
	// Basically: The lock prefix is faster and acts as a memory barrier just as well as mfence
	__asm__ volatile("lock orl $0, (%%esp)" ::: "memory");
}

#endif /* _ATOMIC_H_ */
