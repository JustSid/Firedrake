//
//  iolibrary.h
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

#ifndef _IOLIBRARY_H_
#define _IOLIBRARY_H_

#include <types.h>
#include <system/elf.h>
#include <container/list.h>
#include <memory/memory.h>
#include <container/list.h>

typedef struct io_library_s
{
	char *name;
	char *path;

	// Dynamic section content
	elf_dyn_t *dynamic;

	const char *strtab;
	size_t 		strtabSize;
	elf_sym_t  *symtab;

	elf_rel_t  *rel, *pltRel;
	elf_rel_t  *rellimit, *pltRellimit;
	elf_rela_t *rela;
	elf_rela_t *relalimit;

	list_t *dependencies;

	uint32_t *hashtab;
	uint32_t *buckets;
	uint32_t *chains;
	uint32_t nbuckets;
	uint32_t nchains;

	// Binary content and info
	offset_t relocBase;

	size_t pages;
	uintptr_t    pmemory;
	vm_address_t vmemory;

	// Init array
	uintptr_t *initArray;
	size_t initArrayCount;

	// Misc
	spinlock_t lock;
	uint32_t refCount;
} io_library_t;

struct io_dependency_s
{
	uint32_t name;
	io_library_t *library;

	struct io_dependency_s *next;
	struct io_dependency_s *prev;
};

io_library_t *io_libraryCreate(const char *path, uint8_t *buffer, size_t length);
io_library_t *io_libraryCreateWithFile(const char *file);

void io_libraryRetain(io_library_t *library);
void io_libraryRelease(io_library_t *library);

bool io_libraryRelocateNonPLT(io_library_t *library);
bool io_libraryRelocatePLT(io_library_t *library);

void io_libraryResolveDependencies(io_library_t *library);

vm_address_t io_libraryResolveAddress(io_library_t *library, vm_address_t address);

elf_sym_t *io_librarySymbolWithAddress(io_library_t *library, vm_address_t address);
elf_sym_t *io_librarySymbolWithName(io_library_t *library, const char *name, uint32_t hash);

void *io_libraryFindSymbol(io_library_t *library, const char *name);

#endif /* _IOLINK_H_ */
