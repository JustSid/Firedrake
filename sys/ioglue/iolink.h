//
//  iolink.h
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

#ifndef _IOLINK_H_
#define _IOLINK_H_

#include <types.h>
#include <system/elf.h>
#include <memory/memory.h>
#include <container/list.h>

typedef struct
{
	list_base_t base;
	char *path;

	// Dynamic section content
	void *dynamic;

	const char *strtab;
	size_t 		strtabSize;
	elf_sym_t  *symtab;

	elf_rel_t  *rel, *pltRel;
	elf_rel_t  *rellimit, *pltRellimit;
	elf_rela_t *rela;
	elf_rela_t *relalimit;

	uint32_t *hashtab;
	uint32_t *buckets;
	uint32_t *chains;
	uint32_t nbuckets;
	uint32_t nchains;

	// Binary content and info
	offset_t relocBase;

	uintptr_t 		pmemory;
	vm_address_t 	vmemory;
	size_t pages;

	// Misc
	uint32_t refCount;
} io_library_t;


io_library_t *io_libraryCreate(const char *path, uint8_t *buffer, size_t length);
io_library_t *io_libraryCreateWithFile(const char *file);

void io_libraryRelease(io_library_t *library);
bool io_libraryRelocateNonPLT(io_library_t *library);
bool io_libraryRelocatePLT(io_library_t *library);

elf_sym_t *io_libraryLookupSymbol(io_library_t *library, const char *name, uint32_t hash);

#endif /* _IOLINK_H_ */
