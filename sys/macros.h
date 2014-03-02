//
//  macros.h
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

#ifndef _MACROS_H_
#define _MACROS_H_

#define __unused      __attribute__((unused))
#define __used        __attribute__((used))
#define __deprecated  __attribute__((deprecated))
#define __unavailable __attribute__((unavailable))

#define __inline   inline __attribute__((__always_inline__))
#define __noinline __attribute__((noinline))

#define __expect_true(x)  __builtin_expect(!!(x), 1)
#define __expect_false(x) __builtin_expect(!!(x), 0)

#ifdef __cplusplus
	#define EXTERNC extern "C"
#else
	#define EXTERNC
#endif /* __cplusplus */

#endif /* _MACROS_H_ */
