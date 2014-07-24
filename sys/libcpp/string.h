//
//  string.h
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

#ifndef _LIBCPP_STRING_H_
#define _LIBCPP_STRING_H_

#include <prefix.h>
#include <libc/stddef.h>
#include <libc/string.h>
#include <libc/stdint.h>

#include "algorithm.h"

namespace std
{
	template<size_t Size>
	class fixed_string
	{
	public:
		static constexpr size_t npos = static_cast<size_t>(UINT32_MAX);

		fixed_string() :
			_length(0)
		{
			_buffer[0] = '\0';
		}
		fixed_string(const char *string)
		{
			assign(string, strlen(string));
		}
		fixed_string(const fixed_string &other)
		{
			assign(other._buffer, other._length);
		}


		fixed_string &operator =(const fixed_string &other)
		{
			assign(other._buffer, other._length);
			return *this;
		}
		fixed_string &operator =(const char *string)
		{
			assign(string, strlen(string));
			return *this;
		}


		bool operator ==(const fixed_string &other) const
		{
			if(_length != other._length)
				return false;

			return (strcmp(other._buffer, _buffer) == 0);
		}
		bool operator ==(const char *string) const
		{
			return (strcmp(string, _buffer) == 0);
		}

		bool operator !=(const fixed_string &other) const
		{
			return !(*this == other);
		}
		bool operator !=(const char *string) const
		{
			return !(*this == string);
		}


		size_t find(const fixed_string &other) const
		{
			return find(other._buffer, _length);
		}
		size_t find(const char *other) const
		{
			return find(other, strlen(other));
		}
		size_t find(const char *other, size_t length) const
		{
			size_t found  = 0;

			char *buffer = _buffer;
			while(*buffer)
			{
				if(found >= length)
				{
					size_t diff = ((buffer - found) - _buffer);
					return diff;
				}

				found = (*buffer == other[found]) ? found + 1 : 0;
				buffer ++;
			}

			return npos;
		}

		const char *c_str() const { return _buffer; }
		size_t length() const { return _length; }

	private:
		void assign(const char *string, size_t length)
		{
			if(!string)
			{
				_buffer[0] = '\0';
				_length = 0;

				return;
			}

			_length = std::min(length, Size);
			strlcpy(_buffer, string, _length);
		}

		char _buffer[Size + 1];
		size_t _length;
	};
}

#endif /* _LIBCPP_STRING_H_ */
