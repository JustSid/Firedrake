//
//  stdint.h
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

#ifndef _LIBKERNEL_STDINT_H_
#define _LIBKERNEL_STDINT_H_

#define NULL (void *)0
#define	bool int

#define	false	0
#define	true	1

// MARK: Fixed size integers
typedef signed char 	int8_t;
typedef short 			int16_t;
typedef int 			int32_t;
typedef long long 		int64_t;

typedef unsigned char 		uint8_t;
typedef unsigned short 		uint16_t;
typedef unsigned int 		uint32_t;
typedef unsigned long long 	uint64_t;

#ifndef	_SIZE_T
#define	_SIZE_T
#ifdef SUBLIME_TEXT // Dirty hack to get the compiler to shut up about unsigned int vs unsigned long
typedef unsigned long size_t;
#else
typedef unsigned int size_t;
#endif
#endif
typedef int32_t  intptr_t;
typedef uint32_t uintptr_t;

// MARK: Fixed size integer constraints
#define INT8_C(v)   (v)
#define INT16_C(v)  (v)
#define INT32_C(v)  (v)
#define INT64_C(v)  (v ## LL)

#define UINT8_C(v)   	(v ## U)
#define UINT16_C(v) 	(v ## U)
#define UINT32_C(v)		(v ## U)
#define UINT64_C(v)		(v ## ULL)

#define INTMAX_C(v)  	(v ## LL)
#define UINTMAX_C(v) 	(v ## ULL)


#define INT8_MAX       INT8_C(127)
#define INT16_MAX      INT16_C(32767)
#define INT32_MAX      INT32_C(2147483647)
#define INT64_MAX      INT64_C(9223372036854775807)

#define INT8_MIN       INT8_C(-128)
#define INT16_MIN      INT16_C(-32768)
#define INT32_MIN      INT32_C((-INT32_MAX-1))
#define INT64_MIN      INT64_C((-INT64_MAX-1))

#define UINT8_MAX      UINT8_C(255)
#define UINT16_MAX     UINT16_C(65535)
#define UINT32_MAX     UINT32_C(4294967295)
#define UINT64_MAX     UINT64_C(18446744073709551615)

#endif /* _LIBKERNEL_STDINT_H_ */
