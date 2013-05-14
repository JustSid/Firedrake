//
//  dlfcn.c
//  linkd
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

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "library.h"

static bool __dlerror_hasError = false;
static char __dlerror_message[512];

char *__dlerror()
{
	if(__dlerror_hasError)
	{
		__dlerror_hasError = false;
		return __dlerror_message;
	}

	return NULL;
}

void *__dlopen(const char *file, int flag)
{
	return library_withName(file, flag);
}

void *__dlsym(void *handle, const char *symbol)
{
	if(handle == RTLD_NEXT)
	{
		library_reportError("RTLD_NEXT not supported");
		return NULL;
	}

	void *entry = library_resolveSymbolWithName(handle, symbol);
	if(!entry)
	{
		const char *name = NULL;
		if(handle == RTLD_DEFAULT)
		{
			name = "RTLD_DEFAULT";
		}
		else
		{
			library_t *library = handle;
			name = library->name;
		}

		library_reportError("Couldn't find symbol %s in %s", symbol, name);
	}

	return entry;
}

void __dlclose(void *handle)
{
	((void)handle);
}



void library_reportError(const char *error, ...)
{
	va_list args;
	va_start(args, error);

	vsnprintf(__dlerror_message, 512, error, args);
	__dlerror_hasError = true;

	va_end(args);
}

void library_dieWithError(const char *error, ...)
{
	((void)error);
	abort();
}
