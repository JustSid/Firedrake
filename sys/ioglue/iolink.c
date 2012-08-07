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

#include <memory/memory.h>
#include <system/assert.h>
#include <system/helper.h>
#include <system/syslog.h>
#include <libc/string.h>

#include "iolink.h"
#include "iostore.h"

elf_sym_t *io_libraryLookupSymbol(io_library_t *library, const char *name, uint32_t hash)
{
	uint32_t symnumber = library->buckets[hash % library->nbuckets];

	while(symnumber != 0)
	{
		elf_sym_t *symbol = library->symtab + symnumber;
		const char *str = library->strtab + symbol->st_name;

		if(strcmp(str, name) == 0)
		{
			return symbol;
		}

		symnumber = library->chains[symnumber];
	}

	return NULL;
}


bool io_libraryRelocateNonPLT(io_library_t *library)
{
	elf_rel_t *rel = library->rel;
	for(; rel<library->rellimit; rel ++)
	{
		elf32_address_t *address = (elf32_address_t *)(library->relocBase + rel->r_offset);
		elf32_address_t target, temp;

		elf_sym_t *symbol;
		io_library_t *container;

		uint32_t symnum = ELF32_R_SYM(rel->r_info);
		uint32_t type = ELF32_R_TYPE(rel->r_info);

		switch(type)
		{	
			case R_386_NONE:
				break;

			case R_386_32:
			case R_386_GLOB_DAT:
				symbol = io_storeFindSymbol(library, symnum, &container);
				if(!symbol)
					return false;

				target = (elf32_address_t)(container->relocBase + symbol->st_value);
				temp = target + *address;

				*address = temp;
				break;

			default:
				dbg("Relocation type: %i\n", type);
				break;
		}
	}

	return true;
}

bool io_libraryRelocatePLT(io_library_t *library)
{
	elf_rel_t *rel = library->pltRel;
	for(; rel<library->pltRellimit; rel ++)
	{
		elf32_address_t *address = (elf32_address_t *)(library->relocBase + rel->r_offset);
		elf32_address_t target;

		elf_sym_t *symbol;
		io_library_t *container;

		assert(ELF32_R_TYPE(rel->r_info) == R_386_JMP_SLOT);

		symbol = io_storeFindSymbol(library, ELF32_R_SYM(rel->r_info), &container);
		if(!symbol)
			return false;

		target = (elf32_address_t)(container->relocBase + symbol->st_value);
		*address = target;
	}

	return true;
}

void io_libraryDigestDynamic(io_library_t *library)
{
	elf_dyn_t *dyn;
	bool usePLTRel  = false;
	bool usePLTRela = false;

	size_t relSize = 0;
	size_t relaSize = 0;

	elf32_address_t pltRel = 0;
	size_t pltRelSize = 0;

	for(dyn = (elf_dyn_t *)library->dynamic; dyn->d_tag != DT_NULL; dyn ++)
	{
		switch(dyn->d_tag)
		{
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
				usePLTRel = (dyn->d_un.d_val == DT_REL);
				usePLTRela = (dyn->d_un.d_val == DT_RELA);
				break;

			default:
				break;
		}
	}

	library->rellimit  = (elf_rel_t *)((uint8_t *)library->rel + relSize);
	library->relalimit = (elf_rela_t *)((uint8_t *)library->rela + relaSize);

	if(usePLTRel)
	{
		library->pltRel = (elf_rel_t *)(library->relocBase + pltRel);
		library->pltRellimit = (elf_rel_t *)(library->relocBase + pltRel + pltRelSize);
	}
	else if(usePLTRela)
	{
		dbg("PLT RELA");
		// TODO: Implement!
	}
}

io_library_t *io_libraryCreate(const char *path, uint8_t *buffer, size_t length)
{
	io_store_t *store = io_storeGetStore();
	io_library_t *library = list_addBack(store->libraries);
	if(library)
	{
		// Setup the library
		memset(library, 0, sizeof(io_library_t));

		library->refCount = 1;
		library->path = halloc(NULL, strlen(path) + 1);

		if(!library->path)
		{
			list_remove(store->libraries, library);
			dbg("iolink: Couldn't allocate enough memory for %s\n", path);

			return NULL;
		}

		strcpy(library->path, path);

		// Get the basic ELF info
		elf_header_t *header = (elf_header_t *)buffer;

		if(strncmp((const char *)header->e_ident, ELF_MAGIC, strlen(ELF_MAGIC)) != 0)
		{
			list_remove(store->libraries, library);
			dbg("iolink: %s is not a valid ELF file!\n", path);

			return NULL;
		}

		if(header->e_type != ET_DYN)
		{
			list_remove(store->libraries, library);
			dbg("iolink: %s is not a dynamic ELF file!\n", path);

			return NULL;
		}


		// Parse the program header
		elf_program_header_t *programHeader = (elf_program_header_t *)(buffer + header->e_phoff);
		elf_program_header_t *ptload[2];

		vm_address_t minAddress = -1;
		vm_address_t maxAddress = 0;

		size_t pages = 0;
		size_t segments = 0;

		// Calculate the needed size
		for(int i=0; i<header->e_phnum; i++) 
		{
			elf_program_header_t *program = &programHeader[i];

			if(program->p_type == PT_LOAD)
			{
				if(program->p_paddr < minAddress)
					minAddress = program->p_paddr;

				if(program->p_paddr + program->p_memsz > maxAddress)
					maxAddress = program->p_paddr + program->p_memsz;

				ptload[segments ++] = program;
			}

			if(program->p_type == PT_DYNAMIC)
			{
				library->dynamic = (void *)program->p_vaddr;
			}
		}


		// Reserve enough memory and copy the .text section
		pages = pageCount(maxAddress - minAddress);

		library->pmemory = pm_alloc(pages);
		if(!library->pmemory)
			goto io_libraryFailed;

		library->vmemory = vm_alloc(vm_getKernelDirectory(), (uintptr_t)library->pmemory, pages, VM_FLAGS_KERNEL);
		if(!library->vmemory)
			goto io_libraryFailed;

		library->relocBase = library->vmemory - minAddress;
		library->pages = pages;

		uint8_t *target = (uint8_t *)library->vmemory;
		uint8_t *source = buffer;

		memset(target, 0, pages * VM_PAGE_SIZE);

		for(int i=0; i<2; i++)
		{
			elf_program_header_t *program = ptload[i];
			memcpy(&target[program->p_vaddr - minAddress], &source[program->p_offset], program->p_filesz);
		}

		// Verify
		if(library->dynamic)
		{
			library->dynamic = (void *)(library->relocBase + ((uintptr_t)library->dynamic));
			io_libraryDigestDynamic(library);
		}

		io_libraryRelocateNonPLT(library);
		io_libraryRelocatePLT(library);

		return library;

	io_libraryFailed:
		io_libraryRelease(library);
		dbg("iolink: Failed to create library!\n");

		return NULL;
	}

	return NULL;
}

io_library_t *io_libraryCreateWithFile(const char *file)
{
	struct multiboot_module_s *module = sys_multibootModuleWithName(file);
	if(module)
	{
		uint8_t *begin = (uint8_t *)module->start;
		uint8_t *end = (uint8_t *)module->end;

		return io_libraryCreate(file, begin, (size_t)(end - begin));
	}

	return NULL;
}

void io_libraryRelease(io_library_t *library)
{
	library->refCount --;

	if(library->refCount == 0)
	{
		if(library->vmemory)
			vm_free(vm_getKernelDirectory(), library->vmemory, library->pages);

		if(library->pmemory)
			pm_free(library->pmemory, library->pages);

		if(library->path)
			hfree(NULL, library->path);

		io_store_t *store = io_storeGetStore();
		list_remove(store->libraries, library);
	}
}
