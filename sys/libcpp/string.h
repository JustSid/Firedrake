//
//  string.h
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2013 by Sidney Just
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
	class string
	{
	public:
		static constexpr size_t npos = static_cast<size_t>(UINT32_MAX);

		string() :
			_buffer(nullptr),
			_length(0),
			_capacity(0)
		{
			assert_buffer(10);
			_buffer[0] = '\0';
		}

		string(const string& other) :
			_buffer(nullptr),
			_length(other._length),
			_capacity(0)
		{
			assert_buffer(other._capacity);
			strcpy(_buffer, other._buffer);
		}

		string(const char *other) :
			_buffer(nullptr),
			_length(strlen(other)),
			_capacity(0)
		{
			assert_buffer(strlen(other));
			strcpy(_buffer, other);
		}

		~string()
		{
			delete [] _buffer;
		}

		
		void append(const string& other)
		{
			append(other._buffer, other._length);
		}

		void append(const char *other)
		{
			append(other, strlen(other));
		}

		void append(const char *other, size_t length)
		{
			assert_buffer(_length + length);
			memcpy(_buffer + _length, other, length);

			_length += length;
			_buffer[_length] = '\0';
		}


		size_t find(const string& other)
		{
			return find(other._buffer);
		}

		size_t find(const char *other)
		{
			size_t found  = 0;
			size_t length = strlen(other);

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


		void reserve(size_t size)
		{
			assert_buffer(size);
		}



		const char *get_data() const
		{
			return _buffer;
		}
		size_t get_capacity() const 
		{
			return _capacity;
		}
		size_t get_length() const
		{
			return _length;
		}

	private:
		void expand_buffer(size_t size)
		{
			char *temp = new char[size];

			if(_buffer)
				memcpy(temp, _buffer, _length + 1);

			delete [] _buffer;
			_buffer   = temp;
			_capacity = size;
		}

		void collapse_buffer(size_t size)
		{
			char *temp = new char[size];

			if(_buffer)
			{
				memcpy(temp, _buffer, size);
				temp[size] = '\0';
			}

			delete [] _buffer;
			_buffer   = temp;
			_capacity = size;
		}

		void assert_buffer(size_t minimum)
		{
			if(minimum > _capacity)
			{
				size_t size = std::max(minimum, _capacity * 2);
				expand_buffer(size);
			}
			else if(minimum < (_capacity / 2))
			{
				size_t size = std::min(minimum, _capacity / 2);
				collapse_buffer(size);
			}
		}

		char *_buffer;
		size_t _capacity;
		size_t _length;
	};
}

#endif /* _LIBCPP_STRING_H_ */
