//
//  loader.c
//  Firedrake
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

#include <libc/string.h>
#include <system/elf.h>
#include <system/helper.h>
#include <system/syslog.h>
#include <vfs/vfs.h>
#include "loader.h"

ld_exectuable_t *ld_exectuableCreateWithData(vm_page_directory_t pdirectory, uint8_t *begin)
{
	elf_header_t *header = (elf_header_t *)begin;

	if(strncmp((const char *)header->e_ident, ELF_MAGIC, strlen(ELF_MAGIC)) != 0)
		return NULL;

	ld_exectuable_t *executable = halloc(NULL, sizeof(ld_exectuable_t));
	if(executable)
	{
		// Initialize the executable
		executable->entry      = header->e_entry;
		executable->pdirectory = pdirectory;

		elf_program_header_t *programHeader = (elf_program_header_t *)(begin + header->e_phoff);
		vm_address_t minAddress = -1;
		vm_address_t maxAddress = 0;

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
		minAddress   = VM_PAGE_ALIGN_DOWN(minAddress);
		size_t pages = VM_PAGE_COUNT(maxAddress - minAddress);

		// Memory allocation
		uint8_t *memory = (uint8_t *)pm_alloc(pages);
		uint8_t *target = (uint8_t *)vm_alloc(vm_getKernelDirectory(), (uintptr_t)memory, pages, VM_FLAGS_KERNEL);
		uint8_t *source = begin;

		memset(target, 0, pages * VM_PAGE_SIZE);

		// Copy the data from the image
		for(int i=0; i<header->e_phnum; i++) 
		{
			elf_program_header_t *program = &programHeader[i];

			if(program->p_type == PT_LOAD)
			{
				memcpy(&target[program->p_vaddr - minAddress], &source[program->p_offset], program->p_filesz);
			}
		}

		vm_free(vm_getKernelDirectory(), (vm_address_t)target, pages);
		vm_mapPageRange(pdirectory, (uintptr_t)memory, minAddress, pages, VM_FLAGS_USERLAND);
		
		executable->pimage = (uintptr_t)memory;
		executable->vimage = (vm_address_t)minAddress;
		executable->pages  = pages;
	}

	return executable;
}

ld_exectuable_t *ld_exectuableCreate(vm_page_directory_t pdirectory)
{
	int error;
	int fd = vfs_open(vfs_getKernelContext(), "/bin/linkd.bin", O_RDONLY, &error);
	if(fd >= 0)
	{
		size_t size  = vfs_seek(vfs_getKernelContext(), fd, 0, SEEK_END, &error);
		size_t pages = VM_PAGE_COUNT(size);
		uint8_t *buffer = mm_alloc(vm_getKernelDirectory(), pages, VM_FLAGS_KERNEL);

		vfs_seek(vfs_getKernelContext(), fd, 0, SEEK_SET, &error);
		
		size_t left = size;
		uint8_t *temp = buffer;

		while(left > 0)
		{
			size_t read = vfs_read(vfs_getKernelContext(), fd, temp, left, &error);

			left -= read;
			temp += read;
		}

		ld_exectuable_t *executable = ld_exectuableCreateWithData(pdirectory, buffer);
		
		mm_free(buffer, vm_getKernelDirectory(), pages);
		vfs_close(vfs_getKernelContext(), fd);

		return executable;
	}

	return NULL;
}

ld_exectuable_t *ld_exectuableCopy(vm_page_directory_t pdirectory, ld_exectuable_t *executable)
{
	ld_exectuable_t *copy = halloc(NULL, sizeof(ld_exectuable_t));
	if(copy)
	{
		copy->pdirectory = pdirectory;
		copy->entry = executable->entry;

		copy->pages = executable->pages;

		copy->pimage = pm_alloc(copy->pages);
		copy->vimage = executable->vimage;

		vm_mapPageRange(pdirectory, copy->pimage, copy->vimage, copy->pages, VM_FLAGS_USERLAND);

		uint8_t *target = (uint8_t *)vm_alloc(vm_getKernelDirectory(), (uintptr_t)copy->pimage, copy->pages, VM_FLAGS_KERNEL);
		uint8_t *source = (uint8_t *)vm_alloc(vm_getKernelDirectory(), (uintptr_t)executable->pimage, copy->pages, VM_FLAGS_KERNEL);

		memcpy(target, source, copy->pages * VM_PAGE_SIZE);

		vm_free(vm_getKernelDirectory(), (vm_address_t)source, copy->pages);
		vm_free(vm_getKernelDirectory(), (vm_address_t)target, copy->pages);
	}

	return copy;
}

void ld_destroyExecutable(ld_exectuable_t *executable)
{
	if(executable->pimage)
		pm_free(executable->pimage, executable->pages);

	if(executable->vimage)
		vm_free(executable->pdirectory, executable->vimage, executable->pages);

	hfree(NULL, executable);
}
