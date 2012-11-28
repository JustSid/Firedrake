//
//  byteorder.h
//  libkernel
//
//  Created by Sidney Just
//  Copyright (c) 2012 by Sidney Just
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

#ifndef _LIBKERNEL_BYTEORDER_H_
#define _LIBKERNEL_BYTEORDER_H_

#include "base.h"
#include "stdint.h"

#define KERN_HOST_LITTLE_ENDIAN
//#define KERN_HOST_BIG_ENDIAN

// General swapping function
kern_inline uint16_t kern_swapInt16(uint16_t val)
{
	__asm__("xchgb %b0, %h0" : "+q" (val));
	return val;
}

kern_inline uint32_t kern_swapInt32(uint32_t val)
{
	__asm__("bswap %0" : "+r" (val));
	return val;
}

kern_inline uint64_t kern_swapInt64(uint64_t val)
{
	union swapArea {
		uint64_t c;
		uint32_t h[2];
	} temp, result;

	temp.c = val;
	result.h[0] = kern_swapInt32(temp.h[1]);
	result.h[1] = kern_swapInt32(temp.h[0]);
	return result.c;
}

// From and to host system
// From and To Big Endianness
kern_inline uint16_t kern_swapBigInt16ToHost(uint16_t val)
{
#ifdef KERN_HOST_BIG_ENDIAN
	return val;
#else
	return kern_swapInt16(val);
#endif
}

kern_inline uint32_t kern_swapBigInt32ToHost(uint32_t val)
{
#ifdef KERN_HOST_BIG_ENDIAN
	return val;
#else
	return kern_swapInt32(val);
#endif
}

kern_inline uint64_t kern_swapBigInt64ToHost(uint64_t val)
{
#ifdef KERN_HOST_BIG_ENDIAN
	return val;
#else
	return kern_swapInt64(val);
#endif
}

kern_inline uint16_t kern_swapHostInt16ToBig(uint16_t val)
{
#ifdef KERN_HOST_BIG_ENDIAN
	return val;
#else
	return kern_swapInt16(val);
#endif
}

kern_inline uint32_t kern_swapHostInt16ToBig(uint32_t val)
{
#ifdef KERN_HOST_BIG_ENDIAN
	return val;
#else
	return kern_swapInt32(val);
#endif
}

kern_inline uint64_t kern_swapHostInt16ToBig(uint64_t val)
{
#ifdef KERN_HOST_BIG_ENDIAN
	return val;
#else
	return kern_swapInt64(val);
#endif
}

// From and To Little Endianess
kern_inline uint16_t kern_swapLittleInt16ToHost(uint16_t val)
{
#ifdef KERN_HOST_LITTLE_ENDIAN
	return val;
#else
	return kern_swapInt16(val);
#endif
}

kern_inline uint32_t kern_swapLittleInt32ToHost(uint32_t val)
{
#ifdef KERN_HOST_LITTLE_ENDIAN
	return val;
#else
	return kern_swapInt32(val);
#endif
}

kern_inline uint64_t kern_swapLittleInt16ToHost(uint64_t val)
{
#ifdef KERN_HOST_LITTLE_ENDIAN
	return val;
#else
	return kern_swapInt64(val);
#endif
}

kern_inline uint16_t kern_swapHostInt16ToLittle(uint16_t val)
{
#ifdef KERN_HOST_LITTLE_ENDIAN
	return val;
#else
	return kern_swapInt16(val);
#endif
}

kern_inline uint32_t kern_swapHostInt32ToLittle(uint32_t val)
{
#ifdef KERN_HOST_LITTLE_ENDIAN
	return val;
#else
	return kern_swapInt32(val);
#endif
}

kern_inline uint64_t kern_swapHostInt16ToLittle(uint64_t val)
{
#ifdef KERN_HOST_LITTLE_ENDIAN
	return val;
#else
	return kern_swapInt64(val);
#endif
}

#endif /* _LIBKERNEL_BYTEORDER_H_ */
