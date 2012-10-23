//
//  trampoline.c
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

#include <system/syslog.h>
#include <system/panic.h>
#include <libc/string.h>
#include "trampoline.h"

ir_trampoline_map_t *ir_trampoline_map = (ir_trampoline_map_t *)IR_TRAMPOLINE_BEGIN;

extern uintptr_t idt_sectionBegin; // idt.S
extern uintptr_t idt_sectionEnd; // idt.S

extern void idt_entry_handler(); // Guess what? idt.S!
extern uint32_t ir_handleInterrupt(uint32_t esp); // interrupts.c


uintptr_t ir_trampolineResolveFrame(vm_address_t frame)
{
	vm_address_t offset = frame - IR_TRAMPOLINE_BEGIN;
	vm_address_t _idt_sectionBegin = (vm_address_t)&idt_sectionBegin;

	return (uintptr_t)_idt_sectionBegin + offset;
}


static inline uint32_t ir_trampolineResolveCall(uint8_t *buffer, uintptr_t function)
{
	uint32_t base = (uint32_t)buffer;
	return function - base;
}

void ir_trampolineFixEntryCall(uint32_t offset, size_t limit)
{
	uint8_t *buffer = &ir_trampoline_map->base[offset];
	size_t index = 0;

	while(*buffer != 0xE8) // Look for the call
	{
		buffer ++;
		index ++;

		if(index >= limit)
			panic("ir_trampoline_fixEntryCall(). Couldn't find call!");
	}
	
	uint32_t *call = (uint32_t *)(buffer + 1);
	*call = ir_trampolineResolveCall((uint8_t *)(call + 1), (uintptr_t)ir_handleInterrupt);
}

bool ir_trampoline_init(void *UNUSED(unused))
{
	// Map the trampoline area into memory
	vm_mapPageRange(vm_getKernelDirectory(), (uintptr_t)IR_TRAMPOLINE_PHYSICAL, (vm_address_t)IR_TRAMPOLINE_BEGIN, IR_TRAMPOLINE_PAGES, VM_FLAGS_KERNEL);

	// Copy the IDT section into the trampoline
	vm_address_t _idt_sectionBegin      = (vm_address_t)&idt_sectionBegin;
	vm_address_t _idt_sectionEnd        = (vm_address_t)&idt_sectionEnd;
	vm_address_t _idt_entryHandler      = (vm_address_t)&idt_entry_handler;

	memcpy(ir_trampoline_map->base, &idt_sectionBegin, _idt_sectionEnd - _idt_sectionBegin);
	ir_trampolineFixEntryCall(_idt_entryHandler - _idt_sectionBegin, _idt_sectionEnd - _idt_entryHandler);

	// Set IDT and GDT straight
	ir_idt_init(ir_trampoline_map->idt, IR_TRAMPOLINE_BEGIN - _idt_sectionBegin);
	gdt_init(ir_trampoline_map->gdt, &ir_trampoline_map->tss);

	return true;
}
