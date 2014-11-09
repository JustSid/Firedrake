//
//  loader.cpp
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2014 by Sidney Just
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

#include <vfs/vfs.h>
#include <libc/string.h>
#include <kern/kprintf.h>
#include "elf.h"
#include "loader.h"

namespace Sys
{
	Executable::Executable(VM::Directory *directory) :
		_directory(directory),
		_state(KERN_SUCCESS),
		_entry(0),
		_physical(0),
		_virtual(0),
		_pages(0)
	{
		int fd;
		VFS::Context *context = VFS::Context::GetKernelContext();

		if((_state = VFS::Open(context, "/bin/test.bin", O_RDONLY, fd)) != KERN_SUCCESS)
			return;

		off_t size;
		size_t left;

		uint8_t *temp;
		uint8_t *buffer = nullptr;

		// Get the size of the file and allocate enough storage
		if((_state = VFS::Seek(context, fd, 0, SEEK_END, size)) != KERN_SUCCESS)
			goto exit;

		left   = size;
		buffer = new uint8_t[size];

		if(!buffer)
		{
			_state = KERN_NO_MEMORY;
			goto exit;
		}

		// Rewind
		if((_state = VFS::Seek(context, fd, 0, SEEK_SET, size)) != KERN_SUCCESS)
			goto exit;

		temp = buffer;

		// Read the file
		while(left > 0)
		{
			size_t read;
			_state = VFS::Read(context, fd, temp, left, read);

			if(_state != KERN_SUCCESS)
				goto exit;

			temp += read;
			left -= read;
		}

		_state = Initialize(buffer);

	exit:
		VFS::Close(context, fd);

		if(buffer)
			delete[] buffer;
	}

	Executable::~Executable()
	{
		if(_virtual)
			_directory->Free(_virtual, _pages);

		if(_physical)
			Sys::PM::Free(_physical, _pages);
	}



	kern_return_t Executable::Initialize(void *data)
	{
		char *begin = static_cast<char *>(data);
		elf_header_t *header = static_cast<elf_header_t *>(data);

		if(strncmp((const char *)header->e_ident, ELF_MAGIC, strlen(ELF_MAGIC)) != 0)
			return KERN_FAILURE;

		if(header->e_phnum == 0)
			return KERN_INVALID_ARGUMENT;


		_entry = header->e_entry;

		elf_program_header_t *programHeader = reinterpret_cast<elf_program_header_t *>(begin + header->e_phoff);

		vm_address_t minAddress = -1;
		vm_address_t maxAddress = 0;

		for(size_t i = 0; i < header->e_phnum; i ++)
		{
			elf_program_header_t *program = programHeader + i;

			if(program->p_type == PT_LOAD)
			{
				if(program->p_paddr < minAddress)
					minAddress = program->p_paddr;

				if(program->p_paddr + program->p_memsz > maxAddress)
					maxAddress = program->p_paddr + program->p_memsz;
			}
		}

		minAddress = VM_PAGE_ALIGN_DOWN(minAddress);
		maxAddress = VM_PAGE_ALIGN_UP(maxAddress);

		_pages = VM_PAGE_COUNT(maxAddress - minAddress);

		kern_return_t result;

		if((result = Sys::PM::Alloc(_physical, _pages)) != KERN_SUCCESS)
		{
			_physical = 0x0;
			return result;
		}

		if((result = _directory->Alloc(_virtual, _physical, _pages, kVMFlagsUserlandRW)) != KERN_SUCCESS)
		{
			_virtual = 0x0;
			return result;
		}
		

		// Copy the date into the physical memory
		vm_address_t kernel;
		if((result = Sys::VM::Directory::GetKernelDirectory()->Alloc(kernel, _physical, _pages, kVMFlagsKernel)) != KERN_SUCCESS)
			return result;
		
		uint8_t *buffer = reinterpret_cast<uint8_t *>(kernel);

		for(size_t i = 0; i < header->e_phnum; i ++)
		{
			elf_program_header_t *program = programHeader + i;

			if(program->p_type == PT_LOAD)
			{
				memcpy(&buffer[program->p_vaddr - minAddress], &begin[program->p_offset], program->p_filesz);
			}
		}

		Sys::VM::Directory::GetKernelDirectory()->Free(kernel, _pages);

		return KERN_SUCCESS;
	}
}
