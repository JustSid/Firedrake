//
//  dyexecutable.c
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

#include <libc/string.h>
#include <bootstrap/multiboot.h>
#include <system/elf.h>
#include <system/helper.h>
#include <system/syslog.h>

#include "dyexecutable.h"

dy_exectuable_t *dy_exectuableCreate(vm_page_directory_t pdirectory, uint8_t *begin, size_t size)
{
	dy_exectuable_t *executable = halloc(NULL, sizeof(dy_exectuable_t));

	if(executable)
	{
		elf_header_t *header = (elf_header_t *)begin;

		if(strncmp((const char *)header->e_ident, ELF_MAGIC, strlen(ELF_MAGIC)) != 0)
		{
			hfree(NULL, executable);
			return NULL;
		}

		executable->useCount   = 1;
		executable->entry      = header->e_entry;
		executable->pdirectory = pdirectory;

		elf_program_header_t *programHeader = (elf_program_header_t *)(begin + header->e_phoff);
		vm_address_t minAddress = -1;
		vm_address_t maxAddress = 0;

		size_t pages = 0;

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
			}
		}

		// Calculate the starting address and the number of pages we need to allocate
		minAddress = round4kDown(minAddress);
		pages = pageCount(maxAddress - minAddress);

		// Memory foo
		uint8_t *memory = (uint8_t *)pm_alloc(pages);
		uint8_t *target = (uint8_t *)vm_alloc(vm_getKernelDirectory(), (uintptr_t)memory, pages, VM_FLAGS_KERNEL);
		uint8_t *source = begin;

		memset(target, 0, pages * VM_PAGE_SIZE);

		for(int i=0; i<header->e_phnum; i++) 
		{
			elf_program_header_t *program = &programHeader[i];

			if(program->p_type == PT_LOAD)
			{
				memcpy(&target[program->p_vaddr - minAddress], &source[program->p_offset], program->p_filesz);
			}
		}

		vm_free(vm_getKernelDirectory(), (vm_address_t)target, pages);
		vm_mapPageRange(pdirectory, (uintptr_t)memory, minAddress, pages, VM_FLAGS_USERLAND_R);

		executable->pimage = (uintptr_t)memory;
		executable->vimage = (vm_address_t)minAddress;
		executable->imagePages = pages;
	}

	return executable;
}

dy_exectuable_t *dy_executableCreateWithFile(vm_page_directory_t pdirectory, const char *file)
{
	struct multiboot_module_s *module = sys_multibootModuleWithName(file);
	if(module)
	{
		uint8_t *begin = (uint8_t *)module->start;
		uint8_t *end = (uint8_t *)module->end;

		return dy_exectuableCreate(pdirectory, begin, (size_t)(end - begin));
	}

	return NULL;
}

dy_exectuable_t *dy_exectuableCopy(vm_page_directory_t pdirectory, dy_exectuable_t *source)
{
	if(!source)
		return NULL;

	dy_exectuable_t *executable = halloc(NULL, sizeof(dy_exectuable_t));
	if(executable)
	{
		source->useCount ++;

		executable->pdirectory = pdirectory;
		executable->useCount   = 1;
		executable->entry      = source->entry;

		executable->source = source;
		executable->pimage = source->pimage;
		executable->vimage = source->vimage;
		executable->imagePages = source->imagePages;

		vm_mapPageRange(pdirectory, executable->pimage, executable->vimage, executable->imagePages, VM_FLAGS_USERLAND_R);
	}

	return executable;
}

void dy_executableRelease(dy_exectuable_t *executable)
{
	executable->useCount --;

	if(executable->useCount == 0)
	{
		if(executable->source)
			dy_executableRelease(executable->source);

		if(executable->pimage)
			pm_free(executable->pimage, executable->imagePages);

		if(executable->vimage)
			vm_free(executable->pdirectory, executable->vimage, executable->imagePages);

		hfree(NULL, executable);
	}
}
