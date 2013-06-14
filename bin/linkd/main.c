//
//  main.c
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

#include <sys/syscall.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include <sys/vm.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "elf/elf.h"
#include "library.h"

extern library_t *__library_main;

void *map_file(const char *file, size_t *tsize)
{
	int fd = open(file, O_RDONLY);
	if(fd >= 0)
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

		close(fd);
		return area;
	}

	return NULL;
}

library_t *map_program(uint8_t *begin, void **entry)
{
	elf_header_t *header = (elf_header_t *)begin;
	elf_program_header_t *programHeader = (elf_program_header_t *)(begin + header->e_phoff);

	size_t pages;

	uint32_t minAddress = -1;
	uint32_t maxAddress = 0;

	elf_dyn_t *dynamic = NULL;

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
			dynamic = (elf_dyn_t *)program->p_vaddr;
	}

	minAddress = VM_PAGE_ALIGN_DOWN(minAddress);
	pages = VM_PAGE_COUNT(maxAddress - minAddress);

	uint8_t *target = mmap((void *)minAddress, pages * VM_PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, 0, 0);
	memset(target, 0, pages * VM_PAGE_SIZE);

	for(int i=0; i<header->e_phnum; i++) 
	{
		elf_program_header_t *program = &programHeader[i];

		if(program->p_type == PT_LOAD)
		{
			memcpy(&target[program->p_vaddr - minAddress], &begin[program->p_offset], program->p_filesz);
		}
	}

	library_t *library = calloc(1, sizeof(library_t));

	library->relocBase = 0;	
	library->dynamic = dynamic;

	library->pages = pages;
	library->region = target;

	library->references = 1;

	if(entry)
		*entry = (void *)header->e_entry;

	return library;
}


void library_digestDynamic(library_t *library);
void library_resolveDependencies(library_t *library);

void _start() __attribute__ ((noreturn));
void _start()
{
	size_t size = 0;
	void *data = map_file("/bin/test.bin", &size);

	if(size)
	{
		void (*entry)();

		__library_main = map_program(data, (void **)&entry);
		munmap(data, size);

		if(__library_main->dynamic)
		{
			library_digestDynamic(__library_main);
			library_resolveDependencies(__library_main);

			library_relocatePLT(__library_main);
			library_relocateNonPLT(__library_main);
		}

		// Hand over execution to the program
		entry();
	}

	syscall(1, 0);
	while(1) {}
}
