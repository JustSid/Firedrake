//
//  ioglued.c
//  Firedrake
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

#include <system/syslog.h>
#include <ioglue/iostore.h>
#include <ioglue/iomodule.h>
#include <scheduler/scheduler.h>

#include "ioglued.h"

const char *ioglued_modules[] = {
	"libps2.so"
};

bool ioglued_loadModule(const char *name)
{
	io_library_t *library = io_storeLibraryWithName(name);
	if(!library)
	{
		library = io_libraryCreateWithFile(name);
		bool result  = io_storeAddLibrary(library);

		if(!result)
			return false;
	}

	io_module_t *module = io_moduleCreateWithLibrary(library);
	if(!module)
		return false;

	return true;
}


void ioglued()
{
	thread_setName(thread_getCurrentThread(), "syslogd");

	size_t size = sizeof(ioglued_modules) / sizeof(const char *);
	for(size_t i=0; i<size; i++)
	{
		const char *name = ioglued_modules[i];
		bool result = ioglued_loadModule(name);
		if(!result)
			dbg("ioglued: Failed to publish \"%s\"\n", name);
	}

	uint32_t workload;
	while(1) 
	{
		workload = 0;

		if(workload == 0)
			sd_yield();
	}
}
