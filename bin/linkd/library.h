//
//  library.h
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

#ifndef _LIBRARY_H_
#define _LIBRARY_H_

#include <sys/types.h>
#include <stdint.h>
#include "elf/elf.h"

struct library_s;

typedef struct dependency_s
{
	uint32_t name;
	struct library_s *library;

	struct dependency_s *next;
} dependency_t;

typedef struct library_s
{
	char *name;
	off_t relocBase;

	elf_dyn_t *dynamic;
	dependency_t *dependency;

	const char *strtab;
	size_t      strtabSize;

	elf_sym_t *symtab;

	elf_rel_t  *rel, *pltRel;
	elf_rel_t  *rellimit, *pltRellimit;
	elf_rela_t *rela;
	elf_rela_t *relalimit;

	elf32_address_t *pltgot;	

	uint32_t *hashtab;
	uint32_t *buckets;
	uint32_t *chains;
	uint32_t nbuckets;
	uint32_t nchains;

	uintptr_t *initArray;
	size_t initArrayCount;

	size_t pages;
	void  *region;

	uint32_t references;
	struct library_s *next;
} library_t;

library_t *library_withName(const char *name, int flags);

void library_patchLinkd(library_t *library);
void library_fixPLTGot(library_t *library);

bool library_relocateNonPLT(library_t *library);
bool library_relocatePLT(library_t *library);
void library_relocatePLTLazy(library_t *library);


elf_sym_t *library_lookupSymbol(library_t *library, uint32_t symNum, library_t **outLib);
elf_sym_t *library_symbolWithName(library_t *library, const char *name, uint32_t hash);
void *library_resolveSymbolWithName(library_t *library, const char *name);

#endif
