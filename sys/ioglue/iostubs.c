//
//  iostubs.c
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

#include <bootstrap/multiboot.h>
#include <memory/memory.h>
#include <system/helper.h>
#include <system/syslog.h>
#include <libc/string.h>
#include "iostubs.h"

io_library_t *__io_kernelLibrary = NULL;
size_t __io_kernelLibrarySymbolCount;

io_library_t *io_kernelLibraryStub()
{
	return __io_kernelLibrary;
}

elf_sym_t *io_findKernelSymbol(const char *name)
{
	elf_sym_t *symbol = __io_kernelLibrary->symtab;
	for(size_t i=0; i<__io_kernelLibrarySymbolCount; i++, symbol++)
	{
		if(symbol->st_name >= __io_kernelLibrary->strtabSize || symbol->st_name == 0)
			continue;

		const char *symname = __io_kernelLibrary->strtab + symbol->st_name;
		if(strcmp(symname, name) == 0)
		{
			return symbol;
		}
	}

	return NULL;
}


bool io_initStubs()
{
	__io_kernelLibrary = halloc(NULL, sizeof(io_library_t));
	if(__io_kernelLibrary)
	{
		struct multiboot_module_s *module = sys_multibootModuleWithName("firedrake");
		if(!module)
			return false;

		elf_header_t *header = (elf_header_t *)module->start;
		elf_section_header_t *section = (elf_section_header_t *)(((char *)header) + header->e_shoff);

		for(elf32_word_t i=0; i<header->e_shnum; i++, section++)
		{
			if(section->sh_type == SHT_STRTAB && section->sh_flags == 0 && i != header->e_shstrndx)
			{
				__io_kernelLibrary->strtab = (const char *)(((char *)header) + section->sh_offset);
				__io_kernelLibrary->strtabSize = section->sh_size;
				continue;
			}

			if(section->sh_type == SHT_SYMTAB)
			{
				__io_kernelLibrary->symtab = (elf_sym_t *)(((char *)header) + section->sh_offset);
				__io_kernelLibrarySymbolCount = (section->sh_size / section->sh_entsize);
			}
		}

		__io_kernelLibrary->relocBase = 0x0;

		return (__io_kernelLibrary->strtab && __io_kernelLibrary->symtab);
	}

	return true;
}

// Actual stub implementation

void IOLog(const char *message)
{
	syslog(LOG_INFO, "IOLog: %s\n", message);
}

