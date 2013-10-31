//
//  trampoline.cpp
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

#include <kern/kprintf.h>
#include <libc/string.h>
#include <machine/gdt.h>
#include <machine/memory/virtual.h>
#include <machine/cme.h>
#include "trampoline.h"
#include "interrupts.h"

BEGIN_EXTERNC
extern uintptr_t idt_begin;
extern uintptr_t idt_end;

void idt_entry_handler();
uint32_t ir_handle_interrupt(uint32_t);
END_EXTERNC

namespace ir
{
	trampoline_map_t *trampoline_map = nullptr;
	
	trampoline_map_t *get_trampoline_map()
	{
		return trampoline_map;
	}

	kern_return_t trampoline_init()
	{
		kern_return_t result;
		vm_address_t vaddress;
		uintptr_t paddress;

		// Allocate space for the trampoline map
		result = pm::alloc(paddress, IR_TRAMPOLINE_PAGES);
		if(result != KERN_SUCCESS)
		{
			kprintf("failed to allocate physical trampoline area");
			return result;
		}

		result = vm::get_kernel_directory()->alloc_limit(vaddress, paddress, IR_TRAMPOLINE_BEGIN, VM_UPPER_LIMIT, IR_TRAMPOLINE_PAGES, VM_FLAGS_KERNEL);
		if(result != KERN_SUCCESS)
		{
			kprintf("failed to allocate trampoline area");
			return result;
		}

		trampoline_map = reinterpret_cast<trampoline_map_t *>(IR_TRAMPOLINE_BEGIN);
		trampoline_map->page_directory = vm::get_kernel_directory()->get_physical_directory();

		// Fix up the idt section
		uintptr_t idtBegin        = reinterpret_cast<uintptr_t>(&idt_begin);
		uintptr_t idtEnd          = reinterpret_cast<uintptr_t>(&idt_end);
		uintptr_t idtEntryHandler = reinterpret_cast<uintptr_t>(&idt_entry_handler);

		memcpy(trampoline_map->buffer, &idt_begin, idtEnd - idtBegin);
		cme_fix_call_simple(trampoline_map->buffer + (idtEntryHandler - idtBegin), idtEnd - idtEntryHandler, reinterpret_cast<void *>(&ir_handle_interrupt));

		// Set up the IDT and GDT
		idt_init(trampoline_map->idt, IR_TRAMPOLINE_BEGIN - idtBegin);
		gdt_init(trampoline_map->gdt, &trampoline_map->tss);

		return KERN_SUCCESS;
	}
}
