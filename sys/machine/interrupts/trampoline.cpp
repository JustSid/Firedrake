//
//  trampoline.cpp
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

#include <kern/kprintf.h>
#include <libc/string.h>
#include <libc/assert.h>
#include <machine/gdt.h>
#include <machine/memory/memory.h>
#include <machine/cme.h>
#include <machine/cpu.h>
#include "trampoline.h"
#include "interrupts.h"

BEGIN_EXTERNC
extern uintptr_t idt_begin;
extern uintptr_t idt_end;
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
		assert(sizeof(trampoline_map_t) <= IR_TRAMPOLINE_PAGES * VM_PAGE_SIZE);

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
		if(result != KERN_SUCCESS || vaddress != IR_TRAMPOLINE_BEGIN)
		{
			kprintf("failed to allocate trampoline area");
			return result;
		}

		trampoline_map = reinterpret_cast<trampoline_map_t *>(IR_TRAMPOLINE_BEGIN);

		// Fix up the idt section
		uintptr_t idtBegin = reinterpret_cast<uintptr_t>(&idt_begin);
		uintptr_t idtEnd   = reinterpret_cast<uintptr_t>(&idt_end);

		size_t size = static_cast<size_t>(idtEnd - idtBegin);
		assert(size <= VM_PAGE_SIZE);

		memcpy(trampoline_map->buffer, &idt_begin, idtEnd - idtBegin);
		return trampoline_init_cpu();
	}

	kern_return_t trampoline_init_cpu()
	{
		cpu_t *cpu = cpu_get_current_cpu_slow();
		trampoline_cpu_data_t *data = &trampoline_map->data[cpu->id];

		cpu->data = data;
		data->page_directory = vm::get_kernel_directory()->get_physical_directory();

		uintptr_t idtBegin = reinterpret_cast<uintptr_t>(&idt_begin);
		idt_init(data->idt, IR_TRAMPOLINE_BEGIN - idtBegin);
		gdt_init(data->gdt, &data->tss);

		// Hacky hackery hack
		uint32_t *esp = mm::alloc<uint32_t>(vm::get_kernel_directory(), 1, VM_FLAGS_KERNEL);
		data->tss.esp0 = reinterpret_cast<uint32_t>(esp) + (VM_PAGE_SIZE - sizeof(cpu_state_t));
		data->tss.ss0  = 0x10;

		return KERN_SUCCESS;
	}
}
