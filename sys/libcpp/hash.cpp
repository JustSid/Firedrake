//
//  hash.cpp
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

#include <libc/stddef.h>

namespace std
{
	namespace __hash
	{
		size_t capacity_primitive[] =
		{
			3, 7, 13, 23, 41, 71, 127, 191, 251, 383, 631, 1087, 1723,
			2803, 4523, 7351, 11959, 19447, 31231, 50683, 81919, 132607,
			214519, 346607, 561109, 907759, 1468927, 2376191, 3845119,
			6221311, 10066421, 16287743, 26354171, 42641881, 68996069,
			111638519, 180634607, 292272623, 472907251
		};
			
		size_t count_primitive[] =
		{
			3, 6, 11, 19, 32, 52, 85, 118, 155, 237, 390, 672, 1065,
			1732, 2795, 4543, 7391, 12019, 19302, 31324, 50629, 81956,
			132580, 214215, 346784, 561026, 907847, 1468567, 2376414,
			3844982, 6221390, 10066379, 16287773, 26354132, 42641916,
			68996399, 111638327, 180634415, 292272755
		};
	}
}
