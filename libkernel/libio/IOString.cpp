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

IORegisterClass(IOString, super);

// Factory methods

IOString *IOString::withString(const IOString *other)
{
	IOString *string = IOString::alloc()->initWithString(other);
	return string->autorelease();
}

IOString *IOString::withCString(const char *cstring)
{
	IOString *string = IOString::alloc()->initWithCString(cstring);
	return string->autorelease();
}

IOString *IOString::withFormat(const char *message, ...)
{
	va_list args;
	va_start(args, message);

	char buffer[1024];
	vsnprintf(buffer, 1024, message, args);

	IOString *string = IOString::alloc()->initWithCString(buffer);

	va_end(args);

	return string->autorelease();
}

// Constructor / Destructor

IOString *IOString::init()
{
	if(super::init())
	{
		_string = 0;
		_length = 0;
	}

	return this;
}

IOString *IOString::initWithString(const IOString *other)
{
	if(init())
	{
		_length = other->_length;
		_string = (char *)IOMalloc(other->_length + 1);

		if(_string)
		{
			strlcpy(_string, other->_string, other->_length);
			return this;
		}
	}

	release();
	return 0;
}

IOString *IOString::initWithCString(const char *cstring)
{
	if(init())
	{
		_length = strlen(cstring);
		_string = (char *)IOMalloc(_length + 1);

		if(_string)
		{
			strlcpy(_string, cstring, _length);
			return this;
		}
	}

	release();
	return 0;
}

IOString *IOString::initWithFormat(const char *cstring, ...)
{
	if(init())
	{
		va_list args;
		va_start(args, cstring);

		char buffer[1024];
		vsnprintf(buffer, 1024, cstring, args);

		_length = strlen(buffer);
		_string = (char *)IOMalloc(_length + 1);

		if(_string)
		{
			strlcpy(_string, buffer, _length);
			va_end(args);

			return this;
		}

		va_end(args);
	}

	release();
	return 0;
}

IOString *IOString::initWithVariadic(const char *format, va_list args)
{
	if(init())
	{
		char buffer[1024];
		vsnprintf(buffer, 1024, format, args);

		_length = strlen(buffer);
		_string = (char *)IOMalloc(_length + 1);

		if(_string)
		{
			strlcpy(_string, buffer, _length);
			va_end(args);

			return this;
		}
	}

	release();
	return 0;
}

void IOString::free()
{
	if(_string)
		IOFree(_string);

	super::free();
}


// Misc

IOString *IOString::description() const
{	
	IOString *copy = IOString::withString(this);
	return copy->autorelease();
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
