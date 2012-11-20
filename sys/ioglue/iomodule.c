//
//  iomodule.c
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

#include <container/atree.h>
#include <system/syslog.h>
#include <system/assert.h>
#include <libc/string.h>

#include "iomodule.h"
#include "iostore.h"

spinlock_t __io_moduleLock = SPINLOCK_INIT_LOCKED;
atree_t *__io_moduleTree = NULL;

int io_moduleAtreeLookup(void *key1, void *key2)
{
	const char *name1 = (const char *)key1;
	const char *name2 = (const char *)key2;

	uint32_t index = 0;
	while(1)
	{
		char char1 = name1[index];
		char char2 = name2[index];

		if(char1 == char2)
		{
			if(char1 == '\0')
				return kCompareEqualTo;

			index ++;
			continue;
		}

		if(char1 > char2)
			return kCompareGreaterThan;

		if(char1 < char2)
			return kCompareLesserThan;
	}
}



bool __io_moduleStart(io_module_t *module)
{
	// Call the init functions
	io_library_t *library = module->library;
	for(size_t i=0; i<library->initArrayCount; i++)
	{
		uintptr_t ptr = (library->initArray[i]);
		if(ptr == 0x0 || ptr == UINT32_MAX)
			continue;

		io_module_init_t init = (io_module_init_t)ptr;
		init();
	}
	
	// Call the kernel entry point
	bool result = module->start(module);
	return result;
}

io_module_t *__io_moduleCreateWithLibrary(io_library_t *library, bool retainLibrary)
{
	io_module_t *module = halloc(NULL, sizeof(io_module_t));
	if(module)
	{
		module->library = library;
		module->module  = NULL;

		module->name = library->name;
		module->references = 1;
		module->lock = SPINLOCK_INIT;

		module->start = (io_module_start_t)io_libraryFindSymbol(library, "_kern_start");
		module->stop  = (io_module_stop_t)io_libraryFindSymbol(library, "_kern_stop");

		if(!module->start || !module->stop)
		{
			dbg("library %s doesn't provide kernel entry points!\n", library->name);
			hfree(NULL, module);

			return NULL;
		}

		if(retainLibrary)
			io_libraryRetain(library);

		atree_insert(__io_moduleTree, module, module->name);
	}

	return module;
}

io_module_t *io_moduleWithName(const char *name)
{
	spinlock_lock(&__io_moduleLock);

	io_module_t *module = (io_module_t *)atree_find(__io_moduleTree, (void *)name);
	if(!module)
	{
		bool createdLibrary = false;
		io_library_t *library = io_storeLibraryWithName(name);

		if(!library)
		{
			library = io_libraryCreateWithFile(name);
			io_storeAddLibrary(library);
			
			createdLibrary = true;
		}

		module = __io_moduleCreateWithLibrary(library, !createdLibrary);
		spinlock_unlock(&__io_moduleLock);

		if(module)
		{
			module->initialized = __io_moduleStart(module);
			if(!module->initialized)
			{
				io_libraryRelease(module->library);
				hfree(NULL, module);

				return NULL;
			}
		}
	}
	else
	{
		spinlock_unlock(&__io_moduleLock);
		io_moduleRetain(module);
	}

	
	return module;
}

void io_moduleRetain(io_module_t *module)
{
	if(!module)
		return;

	spinlock_lock(&module->lock);
	module->references ++;
	spinlock_unlock(&module->lock);
}

extern void __ioglued_addReferencelessModule(io_module_t *module);

void io_moduleRelease(io_module_t *module)
{
	if(!module)
		return;

	spinlock_lock(&module->lock);

	if((-- module->references) == 0)
	{
		__ioglued_addReferencelessModule(module);
	}

	spinlock_unlock(&module->lock);
}

void io_moduleStop(io_module_t *module)
{
	if(!module->stop(module))
	{
		module->references ++;
		return;
	}

	spinlock_lock(&__io_moduleLock);
	atree_remove(__io_moduleTree, (void *)module->name);
	spinlock_unlock(&__io_moduleLock);

	io_libraryRelease(module->library);
	hfree(NULL, module);
}


bool io_moduleInit()
{
	__io_moduleTree = atree_create(io_moduleAtreeLookup);
	spinlock_unlock(&__io_moduleLock);

	return (__io_moduleTree != NULL);
}
