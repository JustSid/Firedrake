//
//  reloc.c
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

#include <assert.h>
#include <stdio.h>
#include "library.h"

void library_bindStart();

void library_fixPLTGot(library_t *library)
{
	library->pltgot[1] = (elf32_address_t)library;
	library->pltgot[2] = (elf32_address_t)&library_bindStart;
}

bool library_relocateNonPLT(library_t *library)
{
	for(elf_rel_t *rel = library->rel; rel<library->rellimit; rel++)
	{
		elf32_address_t *address = (elf32_address_t *)(library->relocBase + rel->r_offset);
		elf32_address_t target;

		elf_sym_t *symbol;
		library_t *container;

		uint32_t symnum = ELF32_R_SYM(rel->r_info);
		uint32_t type   = ELF32_R_TYPE(rel->r_info);

		elf_sym_t *lookup = library->symtab + symnum;
		const char *name  = library->strtab + lookup->st_name;

		switch(type)
		{	
			case R_386_NONE:
				break;

			case R_386_32:
			case R_386_GLOB_DAT:
				symbol = library_lookupSymbol(library, symnum, &container);
				if(!symbol)
				{
					library_reportError("Couldn't find symbol %s for %s!", name, library->name);
					return false;
				}

				target = (elf32_address_t)(container->relocBase + symbol->st_value);
				*address = target + *address;
				break;

			case R_386_PC32:
				symbol = library_lookupSymbol(library, symnum, &container);
				if(!symbol)
				{
					library_reportError("Couldn't find symbol %s for %s!", name, library->name);
					return false;
				}

				target = (elf32_address_t)(container->relocBase + symbol->st_value);
				*address += target - (elf32_address_t)address;
				break;

			case R_386_RELATIVE:
				*address += container->relocBase;
				break;

			default:
				printf("Relocation type %i, name %s\n", type, name);
				break;
		}
	}

	return true;
}


bool library_relocatePLTEntry(library_t *library, elf_rel_t *rel, elf32_address_t *outTarget)
{
	elf32_address_t *address = (elf32_address_t *)(library->relocBase + rel->r_offset);
	elf32_address_t target;

	elf_sym_t *symbol;
	library_t *container;

	uint32_t symnum = ELF32_R_SYM(rel->r_info);
	uint32_t type   = ELF32_R_TYPE(rel->r_info);

	assert(type == R_386_JMP_SLOT);

	elf_sym_t *lookup = library->symtab + symnum;
	const char *name  = library->strtab + lookup->st_name;

	symbol = library_lookupSymbol(library, symnum, &container);
	if(!symbol)
	{
		library_reportError("Couldn't find symbol %s for %s!", name, library->name);
		return false;
	}

	target = (elf32_address_t)(container->relocBase + symbol->st_value);
	*address = target;

	if(outTarget)
		*outTarget = target;

	return true;
}

elf32_address_t library_bind(library_t *library, elf32_word_t offset)
{
	elf_rel_t *rel = (elf_rel_t *)((uint8_t *)library->pltRel + offset);
	elf32_address_t target = 0;

	bool result = library_relocatePLTEntry(library, rel, &target);
	if(!result)
		library_dieWithError("Couldn't bind symbol");

	return target;
}

bool library_relocatePLT(library_t *library)
{
	for(elf_rel_t *rel = library->pltRel; rel<library->pltRellimit; rel ++)
	{
		if(!library_relocatePLTEntry(library, rel, NULL))
		{
			return false;
		}
	}

	return true;
}

void library_relocatePLTLazy(library_t *library)
{
	for(elf_rel_t *rel = library->pltRel; rel<library->pltRellimit; rel ++)
	{
		uint32_t type = ELF32_R_TYPE(rel->r_info);
		assert(type == R_386_JMP_SLOT);

		elf32_address_t *address = (elf32_address_t *)(library->relocBase + rel->r_offset);
		*address += (elf32_address_t)library->relocBase;
	}
}
