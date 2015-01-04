//
//  kern_return.cpp
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

#include <objects/IOString.h>
#include "kern_return.h"
#include "panic.h"

namespace OS
{
	int ErrnoFromCode(uint32_t value)
	{
		switch(value)
		{
			case KERN_INVALID_ADDRESS:
				return EFAULT;
			case KERN_INVALID_ARGUMENT:
				return EINVAL;
			case KERN_NO_MEMORY:
				return ENOMEM;
			case KERN_FAILURE:
				return EAGAIN; // KERN_FAILURE translates not well to errno...
			case KERN_RESOURCES_MISSING: // Neither does KERN_RESOURCES_MISSING
				return ENOMEM;
			case KERN_RESOURCE_IN_USE:
				return EADDRINUSE;
			case KERN_RESOURCE_EXISTS:
				return EEXIST;
			case KERN_RESOURCE_EXHAUSTED:
				return EAGAIN;

			default:
				panic("Unknown error code (%d)", value);
		}
	}
}




Error::Error(uint32_t code, const char *description, ...) :
	_code(code),
	_description(nullptr)
{
	va_list args;
	va_start(args, description);
	_description = IO::String::Alloc()->InitWithFormatAndArguments(description, args);
	va_end(args);
}

Error::Error(uint32_t code, uint32_t errno, const char *description, ...) : 
	_code((errno << 24) | (code & 0xffffff)),
	_description(nullptr)
{
	va_list args;
	va_start(args, description);
	_description = IO::String::Alloc()->InitWithFormatAndArguments(description, args);
	va_end(args);
}

Error::Error(const Error &other) :
	_code(other._code),
	_description(IO::SafeRetain(other._description))
{}

Error::~Error()
{
	IO::SafeRelease(_description);
}

Error &Error::operator =(const Error &other)
{
	_code = other._code;
	_description = IO::SafeRetain(other._description);

	return *this;
}

uint32_t Error::GetErrno() const
{
	int errno = (_code >> 24);
	if(errno)
		return errno;

	return OS::ErrnoFromCode(_code);
}

const char *Error::GetDescription() const
{
	return _description ? _description->GetCString() : nullptr;
}