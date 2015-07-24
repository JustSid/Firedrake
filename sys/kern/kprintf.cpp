//
//  kprintf.cpp
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

#include <prefix.h>
#include <libc/stdio.h>
#include <libc/stdarg.h>
#include <libc/stdint.h>
#include <libc/string.h>
#include <libcpp/algorithm.h>

#include "kprintf.h"

#define kMaxOutputHandler 8

namespace Sys
{
	static OutputHandler __outHandler[kMaxOutputHandler];
	static size_t __outHandlerCount = 0;

	void AddOutputHandler(OutputHandler handler)
	{
		if(__outHandlerCount == kMaxOutputHandler)
			return;

		__outHandler[__outHandlerCount ++] = handler;
	}
	void RemoveOutputHandler(OutputHandler handler)
	{
		for(size_t i = 0; i < __outHandlerCount; i ++)
		{
			if(__outHandler[i] == handler)
			{
				for(size_t j = i; j < __outHandlerCount - 1; j ++)
					__outHandler[j] = __outHandler[j + 1];

				__outHandlerCount --;
				return;
			}
		}
	}
}

void kprintf(const char *format, ...)
{
	if(__expect_false(Sys::__outHandlerCount == 0))
		return;

	va_list args;
	va_start(args, format);

	char buffer[512];
	vsnprintf(buffer, 512, format, args);

	size_t length = strlen(buffer);

	for(size_t i = 0; i < Sys::__outHandlerCount; i ++)
	{
		if(Sys::__outHandler[i])
			Sys::__outHandler[i](buffer, length);
	}

	va_end(args);
}

void kputs(const char *string)
{
	if(__expect_false(Sys::__outHandlerCount == 0))
		return;

	size_t length = strlen(string);

	for(size_t i = 0; i < Sys::__outHandlerCount; i ++)
	{
		if(Sys::__outHandler[i])
			Sys::__outHandler[i](string, length);
	}
}

void knputs(const char *string, unsigned int length)
{
	if(__expect_false(Sys::__outHandlerCount == 0))
		return;

	length = strnlen_np(string, length);

	for(size_t i = 0; i < Sys::__outHandlerCount; i ++)
	{
		if(Sys::__outHandler[i])
			Sys::__outHandler[i](string, length);
	}
}
