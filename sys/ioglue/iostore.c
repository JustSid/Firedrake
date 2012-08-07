//
//  iostore.h
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

#include <memory/memory.h>
#include <system/syslog.h>
#include <libc/string.h>

#include "iostore.h"
#include "iostubs.h"

io_store_t *__io_store = NULL;

elf_sym_t *io_storeFindLibraryWithSymbol(const char *name, uint32_t hash, io_library_t **outLib)
{
	elf_sym_t *symbol = io_findKernelSymbol(name);
	if(symbol)
	{
		if(outLib)
			*outLib = io_kernelLibraryStub();

		return symbol;
	}

	list_base_t *entry = list_first(__io_store->libraries);
	while(entry)
	{
		symbol = io_libraryLookupSymbol((io_library_t *)entry, name, hash);
		if(symbol)
		{
			if(outLib)
				*outLib = (io_library_t *)entry;

			return symbol;
		}

		entry = entry->next;
	}

	if(outLib)
		*outLib = NULL;

	return NULL;
}

elf_sym_t *io_storeFindSymbol(io_library_t *library, uint32_t symbol, io_library_t **outLib)
{
	io_library_t *container = NULL;
	elf_sym_t *defintion = NULL;
	elf_sym_t *reference = library->symtab + symbol;

	const char *name = library->strtab + reference->st_name;
	uint32_t hash;

	if(ELF32_ST_BIND(reference->st_info) != STB_LOCAL)
	{
		hash = elf_hash(name);
		defintion = io_storeFindLibraryWithSymbol(name, hash, &container);
	}
	else
	{
		defintion = reference;
		container = library;
	}

	if(defintion)
	{
		if(outLib)
			*outLib = container;

		return defintion;
	}

	dbg("iolink: Failed to lookup symbol '%s'\n", name);
	return NULL;
}


void *io_storeLookupSymbol(const char *name)
{
	io_library_t *container;
	elf_sym_t *defintion;

	uint32_t hash = elf_hash(name);
	defintion = io_storeFindLibraryWithSymbol(name, hash, &container);

	if(defintion && container)
	{
		uint32_t address = container->relocBase + defintion->st_value;
		return (void *)address;
	}

	return NULL;
}

void *io_storeLookupSymbolInLibrary(io_library_t *library, const char *name)
{
	uint32_t hash = elf_hash(name);
	elf_sym_t *defintion = io_libraryLookupSymbol(library, name, hash);

	if(defintion)
	{
		uint32_t address = library->relocBase + defintion->st_value;
		return (void *)address;
	}

	return NULL;
}



io_library_t *io_storeLibraryWithName(const char *name)
{
	list_base_t *entry = list_first(__io_store->libraries);
	while(entry)
	{
		io_library_t *library = (io_library_t *)entry;

		if(strcmp(library->path, name) == 0)
			return library;

		entry = entry->next;
	}

	return NULL;
}

io_store_t *io_storeGetStore()
{
	return __io_store;
}


bool io_init(void *unused)
{
	bool result = io_initStubs();
	if(!result)
		return false;

	__io_store = halloc(NULL, sizeof(io_store_t));

	if(__io_store)
	{
		__io_store->lock = SPINLOCK_INIT;
		__io_store->libraries = list_create(sizeof(io_library_t));


		io_library_t *libkernelso = io_libraryCreateWithFile("libkernel.so"); // This is the essential kernel library which MUST be present
		return (libkernelso != NULL);
	}

	return false;
}
