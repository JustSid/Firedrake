//
//  IOString.cpp
//  libio
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

#include <libc/string.h>
#include <libc/stdio.h>

#include "IOString.h"
#include "IOMemory.h"

#ifdef super
#undef super
#endif
#define super IOObject

IORegisterClass(IOString, super)

bool IOString::init()
{
	_string = 0;
	_length = 0;

	return super::init();
}

void IOString::free()
{
	if(_string)
		IOFree(_string);

	super::free();
}



bool IOString::initWithString(const IOString *other)
{
	_string = 0;
	_length = 0;

	if(!super::init())
		return false;
	
	_string = (char *)IOMalloc(other->_length + 1);
	if(_string)
	{
		strlcpy(_string, other->_string, other->_length);
		_length = other->_length;

		return true;
	}

	return false;
}

bool IOString::initWithCString(const char *cstring)
{
	_string = 0;
	_length = 0;

	if(!super::init())
		return false;
	
	_length = strlen(cstring);
	_string = (char *)IOMalloc(_length + 1);

	if(_string)
	{
		strlcpy(_string, cstring, _length);
		return true;
	}

	return false;
}

bool IOString::initWithFormat(const char *cstring, ...)
{
	va_list args;
	va_start(args, cstring);

	bool result = initWithVariadic(cstring, args);
	va_end(args);

	return result;
}

bool IOString::initWithVariadic(const char *format, va_list args)
{
	char buffer[1024];
	size_t written = vsnprintf(buffer, 1024, format, args);

	if(written > 0)
		return IOString::initWithCString(buffer);
	
	return false;
}



IOString *IOString::withString(const IOString *other)
{
	IOString *string = IOString::alloc();
	if(string)
	{
		bool result = string->initWithString(other);
		if(!result)
		{
			string->release();
			return 0;
		}
	}

	return string;
}

IOString *IOString::withCString(const char *cstring)
{
	IOString *string = IOString::alloc();
	if(string)
	{
		bool result = string->initWithCString(cstring);
		if(!result)
		{
			string->release();
			return 0;
		}
	}

	return string;
}

IOString *IOString::withFormat(const char *message, ...)
{
	IOString *string = IOString::alloc();
	if(string)
	{
		va_list args;
		va_start(args, message);

		bool result = string->initWithVariadic(message, args);
		if(!result)
		{
			string->release();
			va_end(args);

			return 0;
		}

		va_end(args);
	}

	return string;
}

IOString *IOString::description() const
{	
	IOString *copy = IOString::withString(this);
	return copy;
}

hash_t IOString::hash() const
{
	uint8_t *content = (uint8_t *)_string;
	hash_t result = _length;

	if(_length <= 16) 
	{
		for(uint32_t i=0; i<_length; i++) 
			result = result * 257 + content[i];
	} 
	else 
	{
		// Hash the first and last 8 bytes
		for(uint32_t i=0; i<8; i++) 
			result = result * 257 + content[i];
		
		for(uint32_t i=_length - 8; i<_length; i++) 
			result = result * 257 + content[i];
	}
	
	return (result << (_length & 31));
}

bool IOString::isEqual(const IOObject *object) const
{
	if(this == object)
		return true;

	if(object->isSubclassOf(symbol()))
	{
		IOString *string = (IOString *)object;
		return (strcmp(_string, string->_string) == 0);
	}

	return false;
}

const char *IOString::CString() const
{
	return (const char *)_string;
}

size_t IOString::length() const
{
	return _length;
}
