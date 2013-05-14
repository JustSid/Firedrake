//
//  patch.c
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

#include <string.h>
#include <stdio.h>
#include "library.h"

char *__dlerror();
void *__dlopen(const char *file, int flag);
void *__dlsym(void *handle, const char *symbol);
void __dlclose(void *handle);

void library_patchLinkd(library_t *library)
{
	// libld.so contains only stubs, not the actual function implementation
	// so linkd.bin will automatically patch the symbol entries here

	if(strcmp(library->name, "libdl.so") == 0)
	{
		elf_sym_t *sym_dlerror = library_symbolWithName(library, "dlerror", elf_hash("dlerror"));
		elf_sym_t *sym_dlopen  = library_symbolWithName(library, "dlopen", elf_hash("dlopen"));
		elf_sym_t *sym_dlsym   = library_symbolWithName(library, "dlsym", elf_hash("dlsym"));
		elf_sym_t *sym_dlclose = library_symbolWithName(library, "dlclose", elf_hash("dlclose"));

		if(sym_dlerror)
		{
			uintptr_t patch = ((uintptr_t)&__dlerror) - library->relocBase;
			sym_dlerror->st_value = patch;
		}

		if(sym_dlopen)
		{
			uintptr_t patch = ((uintptr_t)&__dlopen) - library->relocBase;
			sym_dlopen->st_value = patch;
		}

		if(sym_dlsym)
		{
			uintptr_t patch = ((uintptr_t)&__dlsym) - library->relocBase;
			sym_dlsym->st_value = patch;
		}

		if(sym_dlclose)
		{
			uintptr_t patch = ((uintptr_t)&__dlclose) - library->relocBase;
			sym_dlclose->st_value = patch;
		}
	}
}
