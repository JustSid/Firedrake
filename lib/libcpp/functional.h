//
//  functional.h
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

#ifndef _FUNCTIONAL_H_
#define _FUNCTIONAL_H_

#include <libc/stdint.h>
#include <libc/string.h>

namespace std
{
	template<class Key>
	struct hash;

	template<class T>
	struct equal_to;

	template<class T>
	void hash_combine(size_t &seed, const T &value)
	{
		std::hash<T> hasher;
		seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}



	template<>
	struct hash<uint32_t>
	{
		size_t operator() (uint32_t value) const
		{
			size_t hash = value * 2654435761;
			hash %= 0x100000000;

			return hash;
		}
	};

	template<>
	struct hash<int32_t>
	{
		size_t operator() (int32_t value) const
		{
			size_t hash = value * 2654435761;
			hash %= 0x100000000;

			return hash;
		}
	};

	template<>
	struct hash<char>
	{
		size_t operator() (char value) const
		{
			size_t hash = static_cast<size_t>(value) * 2654435761;
			hash %= 0x100000000;

			return hash;
		}
	};

	template<>
	struct hash<const char *>
	{
		size_t operator() (const char *value) const
		{
			size_t hash = 0;
			size_t length = strlen(value);

			for(size_t i = 0; i < length; i ++)
			{
				hash_combine(hash, static_cast<int32_t>(value[i]));
			}

			return hash;
		}
	};



	template<>
	struct equal_to<uint32_t>
	{
	public:
		bool operator() (uint32_t a, uint32_t b) const
		{
			return (a == b);
		}
	};

	template<>
	struct equal_to<int32_t>
	{
	public:
		bool operator() (int32_t a, int32_t b) const
		{
			return (a == b);
		}
	};

	template<>
	struct equal_to<const char *>
	{
	public:
		bool operator() (const char *a, const char *b) const
		{
			if(!a || !b)
				return (a == b);

			return (strcmp(a, b) == 0);
		}
	};
}

#endif /* _FUNCTIONAL_H_ */
