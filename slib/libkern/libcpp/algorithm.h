//
//  algorithm.h
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

#ifndef _ALGORITHM_H_
#define _ALGORITHM_H_

namespace std
{
	template<class T>
	const T &min(const T &a, const T &b)
	{
		return (a < b) ? a : b;
	}

	template<class T>
	const T &max(const T &a, const T &b)
	{
		return (a > b) ? a : b;
	}

	template<class ForwardIt, class T>
	void fill(ForwardIt first, ForwardIt last, const T &value)
	{
		while(first != last)
		{
			*first ++ = value;
		}
	}

	template<class InputIt, class OutputIt>
	OutputIt copy(InputIt first, InputIt last, OutputIt dest)
	{
		while(first != last)
			*dest ++ = *first ++;

		return dest;
	}

	template<class InputIt, class T>
	InputIt find(InputIt first, InputIt last, const T &value)
	{
		while(first != last)
		{
			if(*first == value)
				return first;

			first ++;
		}

		return last;
	}
}

#endif /* _ALGORITHM_H_ */
