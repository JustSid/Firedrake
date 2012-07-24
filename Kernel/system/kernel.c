//
//  kernel.c
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

#include <dylink/elf.h>
#include <libc/backtrace.h>
#include <libc/stdio.h>
#include <interrupts/interrupts.h>
#include <interrupts/trampoline.h>
#include "assert.h"
#include "kernel.h"
#include "helper.h"
#include "syslog.h"

struct multiboot_module_s *kern_kernelModule = NULL;
elf_section_header_t *kern_stringTable = NULL;
elf_section_header_t *kern_symbolTable = NULL;


bool kern_fetchStringTable()
{
	elf_header_t *header = (elf_header_t *)kern_kernelModule->start;
	elf_section_header_t *section = (elf_section_header_t *)(((char *)header) + header->e_shoff);

	for(elf32_word_t i=0; i<header->e_shnum; i++, section++)
	{
		if(section->sh_type == SHT_STRTAB && section->sh_flags == 0 && i != header->e_shstrndx)
		{
			kern_stringTable = section;
			return true;
		}
	}

	return false;
}

bool kern_fetchSymbolTable()
{
	elf_header_t *header = (elf_header_t *)kern_kernelModule->start;
	elf_section_header_t *section = (elf_section_header_t *)(((char *)header) + header->e_shoff);

	for(elf32_word_t i=0; i<header->e_shnum; i++, section++)
	{
		if(section->sh_type == SHT_SYMTAB)
		{
			kern_symbolTable = section;
			return true;
		}
	}

	return false;
}



const char *kern_nameForAddress(uintptr_t address)
{
	if(kern_kernelModule == NULL || kern_stringTable == NULL || kern_symbolTable == NULL)
		return "???";

	if(address >= IR_TRAMPOLINE_BEGIN)
	{
		address = ir_trampolineResolveFrame((vm_address_t)address);
		address = kern_resolveAddress(address);
	}

	// Look for the matching symbol!
	elf_header_t *header = (elf_header_t *)kern_kernelModule->start;
	elf_sym_t *symbol = (elf_sym_t *)(((char *)header) + kern_symbolTable->sh_offset);

	size_t symbols = kern_symbolTable->sh_size / kern_symbolTable->sh_entsize;

	for(size_t i=0; i<symbols; i++, symbol++)
	{
		if(symbol->st_value == address)
		{
			if(symbol->st_name >= kern_stringTable->sh_size)
				return "<corrupt>";

			char *name = (symbol->st_name) ? ((char *)header) + kern_stringTable->sh_offset + symbol->st_name : "<null>";
			return name;
		}
	}

	// Errr...
	return "???";
}

uintptr_t kern_resolveAddress(uintptr_t address)
{
	if(kern_kernelModule == NULL || kern_symbolTable == NULL)
		return 0x0;

	if(address >= IR_TRAMPOLINE_BEGIN)
	{
		address = ir_trampolineResolveFrame((vm_address_t)address);
	}

	// Look for the matching symbol!
	elf_header_t *header = (elf_header_t *)kern_kernelModule->start;
	elf_sym_t *tsymbol = (elf_sym_t *)(((char *)header) + kern_symbolTable->sh_offset);

	size_t symbols = kern_symbolTable->sh_size / kern_symbolTable->sh_entsize;

	while(address > 0x100000)
	{
		elf_sym_t *symbol = tsymbol;

		for(size_t i=0; i<symbols; i++, symbol++)
		{
			if(symbol->st_value == address)
				return address;
		}

		address --;
	}

	// Errr...
	return 0x0;	
}



void kern_printBacktrace(long depth)
{
	if(!kern_kernelModule)
		kern_kernelModule = sys_multibootModuleWithName("firedrake");

	if(!kern_stringTable)
		kern_fetchStringTable();

	if(!kern_symbolTable)
		kern_fetchSymbolTable();


	void *addresses[depth];
	size_t size = backtrace(addresses, depth);

	for(size_t i=1; i<size; i++)
	{
		size_t level = (size - 1) - i;
		uintptr_t uaddress = (uintptr_t)addresses[i];
		uintptr_t address = kern_resolveAddress(uaddress);

		dbg("(%2i) %08x: ", level, address);
		info("%s\n", kern_nameForAddress(address));
	}
}


void kern_setWatchpoint(uint8_t reg, bool global, uintptr_t address, kern_breakCondition condition, kern_watchBytes bytes)
{
	assert(reg <= 3);

	uint32_t dr7;
	__asm__ volatile("mov %%dr7, %0" : "=r" (dr7));

	// Clear out the bits for the given register
	dr7 &= ~ (0x3 << (0 + reg));
	dr7 &= ~ (0xF << (16 + (reg * 4)));

	// Update the address
	dr7 |= (1 << (0 + reg + (global ? 1 : 0)));

	// Update the breakpoint condition
	dr7 |= ((char)condition << (16 + (reg * 4)));

	// Update the watch size
	dr7 |= ((char)bytes << (18 + (reg * 4)));

	switch(reg)
	{
		case 0:
			__asm__ volatile("mov %0, %%dr0" : : "r" (address));
			break;

		case 1:
			__asm__ volatile("mov %0, %%dr1" : : "r" (address));
			break;

		case 2:
			__asm__ volatile("mov %0, %%dr2" : : "r" (address));
			break;

		case 3:
			__asm__ volatile("mov %0, %%dr3" : : "r" (address));
			break;

		default:
			break;
	}

	__asm__ volatile("mov %0, %%dr7" : : "r" (dr7));
}

void kern_disableWatchpoint(uint8_t reg)
{
	uint32_t dr7;
	__asm__ volatile("mov %%dr7, %0" : "=r" (dr7));

	dr7 &= ~ (0x3 << (0 + reg));
	dr7 &= ~ (0xF << (16 + (reg * 4)));

	__asm__ volatile("mov %0, %%dr7" : : "r" (dr7));
}
