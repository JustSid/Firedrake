//
//  library.c
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

#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include <sys/lock.h>
#include <sys/vm.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "library.h"

library_t *__library_first = NULL;
library_t *__library_main  = NULL;
spinlock_t __library_lock = SPINLOCK_INIT;

void library_resolveDependencies(library_t *library)
{
	dependency_t *dep = library->dependency;
	while(dep)
	{
		if(!dep->library)
		{
			const char *name = dep->name + library->strtab;
			if(strstr((char *)name, "libdl.so") == name)
			{
				dep->library = library_withName("libdl.so", RTLD_GLOBAL | RTLD_NOW);
				dep->library->references ++;
			}
			else
			{
				dep->library = library_withName(name, RTLD_GLOBAL | RTLD_NOW);
				dep->library->references ++;
			}
		}

		dep = dep->next;
	}
}

void library_digestDynamic(library_t *library)
{
	elf_dyn_t *dyn;
	bool usePLTRel  = false;
	bool usePLTRela = false;

	size_t relSize = 0;
	size_t relaSize = 0;

	elf32_address_t pltRel = 0;
	size_t pltRelSize = 0;

	dependency_t *lastDep = NULL;

	for(dyn=library->dynamic; dyn->d_tag != DT_NULL; dyn ++)
	{
		switch(dyn->d_tag)
		{
			case DT_NEEDED:
			{
				dependency_t *dependency = malloc(sizeof(dependency_t));
				dependency->name = dyn->d_un.d_val;
				dependency->library = NULL;
				dependency->next = NULL;

				if(lastDep)
				{
					lastDep->next = dependency;
					lastDep = dependency;
				}
				else
				{
					lastDep = dependency;
					library->dependency = dependency;
				}

				break;
			}

			case DT_REL:
				library->rel = (elf_rel_t *)(library->relocBase + dyn->d_un.d_ptr);
				break;

			case DT_RELSZ:
				relSize = dyn->d_un.d_val;
				break;

			case DT_JMPREL:
				pltRel = dyn->d_un.d_ptr;
				break;

			case DT_PLTRELSZ:
				pltRelSize = dyn->d_un.d_val;
				break;

			case DT_RELENT:
				assert(dyn->d_un.d_val == sizeof(elf_rel_t));
				break;

			case DT_RELA:
				library->rela = (elf_rela_t *)(library->relocBase + dyn->d_un.d_ptr);
				break;

			case DT_RELASZ:
				relaSize = dyn->d_un.d_val;
				break;

			case DT_STRTAB:
				library->strtab = (const char *)(library->relocBase + dyn->d_un.d_ptr);
				break;

			case DT_STRSZ:
				library->strtabSize = dyn->d_un.d_val;
				break;

			case DT_SYMTAB:
				library->symtab = (elf_sym_t *)(library->relocBase + dyn->d_un.d_ptr);
				break;

			case DT_SYMENT:
				assert(dyn->d_un.d_val == sizeof(elf_sym_t));
				break;

			case DT_HASH:
				library->hashtab  = (uint32_t *)(library->relocBase + dyn->d_un.d_ptr);

				library->nbuckets = library->hashtab[0];
				library->nchains  = library->hashtab[1];
				
				library->buckets  = library->hashtab + 2;
				library->chains   = library->buckets + library->nbuckets;
				break;

			case DT_PLTREL:
				usePLTRel  = (dyn->d_un.d_val == DT_REL);
				usePLTRela = (dyn->d_un.d_val == DT_RELA);
				break;

			case DT_INIT_ARRAY:
				library->initArray = (uintptr_t *)(library->relocBase + dyn->d_un.d_ptr);
				break;

			case DT_INIT_ARRAYSZ:
				library->initArrayCount = (dyn->d_un.d_val / sizeof(uintptr_t));
				break;

			default:
				break;
		}
	}

	// Relocation
	library->rellimit  = (elf_rel_t *)((uint8_t *)library->rel + relSize);
	library->relalimit = (elf_rela_t *)((uint8_t *)library->rela + relaSize);

	if(usePLTRel)
	{
		library->pltRel = (elf_rel_t *)(library->relocBase + pltRel);
		library->pltRellimit = (elf_rel_t *)(library->relocBase + pltRel + pltRelSize);
	}
	else if(usePLTRela)
	{
		printf("PLT RELA");
		// TODO: Implement!
	}
}

// ------------
// Lookup
// ------------

elf_sym_t *library_lookupSymbol(library_t *library, uint32_t symNum, library_t **outLib)
{
	elf_sym_t *lookup = library->symtab + symNum;
	const char *name  = library->strtab + lookup->st_name;

	if(ELF32_ST_BIND(lookup->st_info) == STB_LOCAL)
	{
		*outLib = library;
		return lookup;
	}
	else
	{
		uint32_t hash = elf_hash(name);
		elf_sym_t *symbol;

		// Look it up in the main program
		symbol = library_symbolWithName(__library_main, name, hash);
		if(symbol && symbol->st_value != 0x0)
		{
			*outLib = __library_main;
			return symbol;
		}

		// Search the dependencies
		// TODO: Breadth first search!
		dependency_t *dep = library->dependency;
		while(dep)
		{
			symbol = library_symbolWithName(dep->library, name, hash);
			if(symbol && symbol->st_value != 0x0)
			{
				*outLib = dep->library;
				return symbol;
			}

			dep = dep->next;
		}

		// Look it up in the library
		symbol = library_symbolWithName(library, name, hash);
		if(symbol && symbol->st_value != 0x0)
		{
			*outLib = library;
			return symbol;
		}
	}

	return NULL;
}

elf_sym_t *library_symbolWithName(library_t *library, const char *name, uint32_t hash)
{
	uint32_t symnum = library->buckets[hash % library->nbuckets];
	while(symnum != 0)
	{
		elf_sym_t *symbol = library->symtab + symnum;
		const char *str = library->strtab + symbol->st_name;

		if(strcmp(str, name) == 0)
			return symbol;

		symnum = library->chains[symnum];
	}

	return NULL;
}

void *library_resolveSymbolWithName(library_t *library, const char *name)
{
	uint32_t hash = elf_hash(name);
	if(!library)
	{
		library = __library_first;
		while(library)
		{
			elf_sym_t *symbol = library_symbolWithName(library, name, hash);
			if(symbol)
				return (void *)(library->relocBase + symbol->st_value);

			library = library->next;
		}
	}
	else
	{
		elf_sym_t *symbol = library_symbolWithName(library, name, hash);
		if(symbol)
			return (void *)(library->relocBase + symbol->st_value);
	}

	return NULL;
}

// ------------
// Library loading
// ------------

int library_fileForName(const char *name)
{
	int fd;
	char buffer[255];

	fd = open(name, O_RDONLY);
	if(fd >= 0)
		goto exitWithFD;

	sprintf(buffer, "/lib/%s", name);
	fd = open(buffer, O_RDONLY);
	if(fd >= 0)
		goto exitWithFD;

exitWithFD:
	return fd;
}

void *library_map_file(int fd, size_t *tsize)
{
	size_t size = lseek(fd, 0, SEEK_END);
	size_t mmapSize = VM_PAGE_COUNT(size) * VM_PAGE_SIZE;

	lseek(fd, 0, SEEK_SET);

	void *area = mmap(NULL, mmapSize, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	uint8_t *buffer = area;

	if(area == MAP_FAILED)
	{
		printf("mmap() failed. Errno: %i\n", errno);
		return NULL;
	}

	while(size > 0)
	{
		size_t bytes = read(fd, buffer, size);
		if(bytes == (size_t)-1)
		{
			printf("Error while reading file. Errno: %i\n", errno);
			munmap(area, mmapSize);
			return NULL;
		}
		
		size -= bytes;
		buffer += bytes;
	}

	if(tsize)
		*tsize = mmapSize;

	return area;
}

void library_map_library(library_t *library, uint8_t *begin)
{
	elf_header_t *header = (elf_header_t *)begin;
	elf_program_header_t *programHeader = (elf_program_header_t *)(begin + header->e_phoff);

	size_t pages;

	uint32_t minAddress = -1;
	uint32_t maxAddress = 0;

	for(int i=0; i<header->e_phnum; i++) 
	{
		elf_program_header_t *program = &programHeader[i];

		if(program->p_type == PT_LOAD)
		{
			if(program->p_paddr < minAddress)
				minAddress = program->p_paddr;

			if(program->p_paddr + program->p_memsz > maxAddress)
				maxAddress = program->p_paddr + program->p_memsz;
		}

		if(program->p_type == PT_DYNAMIC)
			library->dynamic = (elf_dyn_t *)program->p_vaddr;
	}

	minAddress = VM_PAGE_ALIGN_DOWN(minAddress);
	pages = VM_PAGE_COUNT(maxAddress - minAddress);

	uint8_t *target = mmap((void *)minAddress, pages * VM_PAGE_SIZE, PROT_READ | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	memset(target, 0, pages * VM_PAGE_SIZE);

	for(int i=0; i<header->e_phnum; i++) 
	{
		elf_program_header_t *program = &programHeader[i];

		if(program->p_type == PT_LOAD)
		{
			memcpy(&target[program->p_vaddr - minAddress], &begin[program->p_offset], program->p_filesz);
		}
	}

	library->relocBase = (off_t)(target - minAddress);

	library->pages = pages;
	library->region = target;

	if(library->dynamic)
	{
		library->dynamic = (elf_dyn_t *)(library->relocBase + ((uintptr_t)library->dynamic));
		library_digestDynamic(library);
	}
}

library_t *library_withName(const char *name, int flags)
{
	library_t *library = __library_first;
	while(library)
	{
		if(strcmp(library->name, name) == 0)
			return library;

		library = library->next;
	}

	int fd = library_fileForName(name);
	if(fd >= 0)
	{
		library_t *library = calloc(1, sizeof(library_t));

		library->references = 1;
		library->name = malloc(strlen(name) + 1);

		strcpy(library->name, name);

		size_t fileSize;
		uint8_t *begin = library_map_file(fd, &fileSize);
		close(fd);

		library_map_library(library, begin);
		munmap(begin, fileSize);

		library_patchLinkd(library);
		library_resolveDependencies(library);

		if(flags & RTLD_NOW)
		{
			library_relocatePLT(library);
			library_relocateNonPLT(library);
		}

		if(flags & RTLD_GLOBAL)
		{
			if(__library_first)
				library->next = __library_first;

			__library_first = library;
		}

		return library;
	}

	return NULL;
}
