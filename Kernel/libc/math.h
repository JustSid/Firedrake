//
//  math.h
//  Firedrake
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

/*
 * Overview:
 * Defines some math helper functions.
 */
#ifndef _MATH_H_
#define _MATH_H_
	 
#include "types.h"

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)

static inline int abs(int i)
{
	return (i < 0) ? -i : i;
}

static inline float fabs(float f)
{
	return (f < 0.0f) ? -f : f;
}

static inline double dabs(double d)
{
	return (d < 0.0) ? -d : d;
}


#define CEILING64_POS(n) ((n-(int64_t)(n)) > 0 ? (int64_t)(n + 1) : (int64_t)(n))
#define CEILING64_NEG(n) ((n-(int64_t)(n)) < 0 ? (int64_t)(n - 1) : (int64_t)(n))
#define CEILING64(n) (((n) > 0) ? CEILING64_POS(n) : CEILING64_NEG(n))

#define CEILING32_POS(n) ((n-(int32_t)(n)) > 0 ? (int32_t)(n + 1) : (int32_t)(n))
#define CEILING32_NEG(n) ((n-(int32_t)(n)) < 0 ? (int32_t)(n - 1) : (int32_t)(n))
#define CEILING32(n) (((n) > 0) ? CEILING32_POS(n) : CEILING32_NEG(n))

static inline double ceil(double v)
{
	return CEILING64(v);
}

static inline float ceilf(float v)
{
	return CEILING32(v);
}

#endif /* _MATH_H_ */
