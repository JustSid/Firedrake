//
//  interrupts.c
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

#include <system/port.h>
#include <system/panic.h>
#include <system/syslog.h>
#include <scheduler/scheduler.h>
#include <libc/string.h>
#include "interrupts.h"
#include "trampoline.h"

// IDT Related stuff
static ir_interrupt_handler_t __ir_interruptHandler[IDT_ENTRIES];
static ir_interrupt_callback_t __ir_interruptCallbacks[IDT_ENTRIES];

void ir_idt_setEntry(uint64_t *idt, uint32_t index, uint32_t handler, uint32_t selector, uint32_t flags)
{
	idt[index] = handler & UINT64_C(0xFFFF);
	idt[index] |= (selector & UINT64_C(0xFFFF)) << 16;
	idt[index] |= (flags & UINT64_C(0xFF)) << 40;
	idt[index] |= ((handler>> 16) & UINT64_C(0xFFFF)) << 48;
}

#define ir_idt_entry(num, flags) \
	do{ ir_idt_setEntry(idt, num, ((uint32_t)idt_interrupt_ ## num) + offset, 0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_PRESENT | flags); } while(0)

void ir_idt_init(uint64_t *idt, uint32_t offset)
{
	// Excpetion-Handler
	ir_idt_setEntry(idt, 0x0, ((uint32_t)idt_exception_divbyzero) + offset, 			0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0x1, ((uint32_t)idt_exception_debug) + offset, 				0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0x3, ((uint32_t)idt_exception_breakpoint) + offset, 			0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0x4, ((uint32_t)idt_exception_overflow) + offset, 				0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0x5, ((uint32_t)idt_exception_boundrange) + offset, 			0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0x6, ((uint32_t)idt_exception_opcode) + offset, 				0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0x7, ((uint32_t)idt_exception_devicenotavailable) + offset, 	0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0x8, ((uint32_t)idt_exception_doublefault) + offset, 			0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0x9, ((uint32_t)idt_exception_segmentoverrun) + offset, 		0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0xA, ((uint32_t)idt_exception_invalidtss) + offset, 			0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0xB, ((uint32_t)idt_exception_segmentnotpresent) + offset, 	0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0xC, ((uint32_t)idt_exception_stackfault) + offset, 			0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0xD, ((uint32_t)idt_exception_protectionfault) + offset, 		0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0xE, ((uint32_t)idt_exception_pagefault) + offset, 			0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0x10, ((uint32_t)idt_exception_fpuerror) + offset, 			0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0x11, ((uint32_t)idt_exception_alignment) + offset, 			0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0x12, ((uint32_t)idt_exception_machinecheck) + offset, 		0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
	ir_idt_setEntry(idt, 0x13, ((uint32_t)idt_exception_simd) + offset, 				0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);

	// Interrupts
	ir_idt_entry(0x02, IDT_FLAG_RING0);
	ir_idt_entry(0x20, IDT_FLAG_RING0);
	ir_idt_entry(0x21, IDT_FLAG_RING0);
	ir_idt_entry(0x22, IDT_FLAG_RING0);
	ir_idt_entry(0x23, IDT_FLAG_RING0);
	ir_idt_entry(0x24, IDT_FLAG_RING0);
	ir_idt_entry(0x25, IDT_FLAG_RING0);
	ir_idt_entry(0x26, IDT_FLAG_RING0);
	ir_idt_entry(0x27, IDT_FLAG_RING0);
	ir_idt_entry(0x28, IDT_FLAG_RING0);
	ir_idt_entry(0x29, IDT_FLAG_RING0);
	ir_idt_entry(0x2A, IDT_FLAG_RING0);
	ir_idt_entry(0x2B, IDT_FLAG_RING0);
	ir_idt_entry(0x2C, IDT_FLAG_RING0);
	ir_idt_entry(0x2D, IDT_FLAG_RING0);
	ir_idt_entry(0x2E, IDT_FLAG_RING0);
	ir_idt_entry(0x2F, IDT_FLAG_RING0);

	// Syscalls
	ir_idt_entry(0x31, IDT_FLAG_RING0);
	ir_idt_entry(0x80, IDT_FLAG_RING3);

	// Free to use syscalls
	ir_idt_entry(0x81, IDT_FLAG_RING0);
	ir_idt_entry(0x82, IDT_FLAG_RING0);
	ir_idt_entry(0x83, IDT_FLAG_RING0);
	ir_idt_entry(0x84, IDT_FLAG_RING0);
	ir_idt_entry(0x85, IDT_FLAG_RING0);
	ir_idt_entry(0x86, IDT_FLAG_RING0);
	ir_idt_entry(0x87, IDT_FLAG_RING0);
	ir_idt_entry(0x88, IDT_FLAG_RING0);
	ir_idt_entry(0x89, IDT_FLAG_RING0);
	ir_idt_entry(0x8A, IDT_FLAG_RING0);
	ir_idt_entry(0x8B, IDT_FLAG_RING0);
	ir_idt_entry(0x8C, IDT_FLAG_RING0);
	ir_idt_entry(0x8D, IDT_FLAG_RING0);
	ir_idt_entry(0x8E, IDT_FLAG_RING0);
	ir_idt_entry(0x8F, IDT_FLAG_RING0);
	ir_idt_entry(0x90, IDT_FLAG_RING0);
	ir_idt_entry(0x91, IDT_FLAG_RING0);
	ir_idt_entry(0x92, IDT_FLAG_RING0);
	ir_idt_entry(0x93, IDT_FLAG_RING0);
	ir_idt_entry(0x94, IDT_FLAG_RING0);
	ir_idt_entry(0x95, IDT_FLAG_RING0);
	ir_idt_entry(0x96, IDT_FLAG_RING0);
	ir_idt_entry(0x97, IDT_FLAG_RING0);
	ir_idt_entry(0x98, IDT_FLAG_RING0);
	ir_idt_entry(0x99, IDT_FLAG_RING0);
	ir_idt_entry(0x9A, IDT_FLAG_RING0);
	ir_idt_entry(0x9B, IDT_FLAG_RING0);
	ir_idt_entry(0x9C, IDT_FLAG_RING0);
	ir_idt_entry(0x9D, IDT_FLAG_RING0);
	ir_idt_entry(0x9E, IDT_FLAG_RING0);
	ir_idt_entry(0x9F, IDT_FLAG_RING0);

	// Reload the IDT
	struct 
	{
		unsigned short int limit;
		void *pointer;
	} __attribute__((packed)) idtp = 
	{
		.limit = (IDT_ENTRIES * sizeof(uint64_t)) - 1,
		.pointer = idt,
	};

	__asm__ volatile("lidt %0" : : "m" (idtp));
}

// PIC

void ir_pic_init()
{
	// Initialize the master pic
	outb(0x20, 0x11);
	outb(0x21, 0x20);
	outb(0x21, 0x04);
	outb(0x21, 0x01);

	// Initialize the slave PIC
	outb(0xA0, 0x11);
	outb(0xA1, 0x28);
	outb(0xA1, 0x02);
	outb(0xA1, 0x01);

	// Demask the interrupts
	outb(0x20, 0x0);
	outb(0xA0, 0x0);
}

// Helper
bool ir_isValidInterrupt(uint32_t interrupt, bool publicOnly)
{
	if(interrupt <= 0x31 || interrupt == 0x80)
		return publicOnly;

	if(interrupt >= 0x81 && interrupt <= 0x9F)
		return true;

	return false;
}

uint32_t ir_interruptPublicBegin()
{
	return 0x81;
}
uint32_t ir_interruptPublicEnd()
{
	return 0x9F;
}

// Interrupt handling
void ir_setInterruptHandler(ir_interrupt_handler_t handler, uint32_t interrupt)
{
	__ir_interruptHandler[interrupt] = handler;
}

void ir_setInterruptCallback(ir_interrupt_callback_t callback, uint32_t interrupt)
{
	__ir_interruptCallbacks[interrupt] = callback;
}


uint32_t __ir_handleException(uint32_t esp);
uint32_t __ir_handleInterrupt(uint32_t esp)
{
	return esp;
}

static uint32_t ir_entries = 0;
static uint32_t ir_lastESP = 0;

uint32_t ir_handleInterrupt(uint32_t esp)
{
	ir_entries ++;
	ir_lastESP = esp;

	cpu_state_t *state = (cpu_state_t *)esp;

	ir_interrupt_handler_t handler = __ir_interruptHandler[state->interrupt];
	ir_interrupt_callback_t callback = __ir_interruptCallbacks[state->interrupt];

	if(!handler && !callback)
		panic("Unhandled interrupt %i!", state->interrupt);

	if(handler)
		esp = handler(esp);

	if(callback)
		callback(state);

	if(state->interrupt >= 0x20 && state->interrupt < 0x30)
	{
		if(state->interrupt >= 0x28)
			outb(0xA0, 0x20);
	
		outb(0x20, 0x20);
	}

	ir_entries --;
	return esp;
}


bool ir_isInsideInterruptHandler()
{
	return (ir_entries > 0);
}

uint32_t ir_lastInterruptESP()
{
	return ir_lastESP;
}


void ir_disableInterrupts(bool disableNMI)
{
	__asm__ volatile("cli;");

	if(disableNMI)
		outb(0x70, inb(0x70) | 0x80);
}

void ir_enableInterrupts(bool enableNMI)
{
	__asm__ volatile("sti;");

	if(enableNMI)
		outb(0x70, inb(0x70) & 0x7F);
}

bool ir_init(void *unused)
{
	ir_setInterruptHandler(__ir_handleException, 0x00);
	ir_setInterruptHandler(__ir_handleException, 0x01);
	ir_setInterruptHandler(__ir_handleException, 0x03);
	ir_setInterruptHandler(__ir_handleException, 0x04);
	ir_setInterruptHandler(__ir_handleException, 0x05);
	ir_setInterruptHandler(__ir_handleException, 0x06);
	ir_setInterruptHandler(__ir_handleException, 0x07);
	ir_setInterruptHandler(__ir_handleException, 0x08);
	ir_setInterruptHandler(__ir_handleException, 0x09);
	ir_setInterruptHandler(__ir_handleException, 0x0A);
	ir_setInterruptHandler(__ir_handleException, 0x0B);
	ir_setInterruptHandler(__ir_handleException, 0x0C);
	ir_setInterruptHandler(__ir_handleException, 0x0D);
	ir_setInterruptHandler(__ir_handleException, 0x0E);
	ir_setInterruptHandler(__ir_handleException, 0x10);
	ir_setInterruptHandler(__ir_handleException, 0x11);
	ir_setInterruptHandler(__ir_handleException, 0x12);
	ir_setInterruptHandler(__ir_handleException, 0x13);

	ir_setInterruptHandler(__ir_handleInterrupt, 0x02);
	ir_setInterruptHandler(__ir_handleInterrupt, 0x20);
	ir_setInterruptHandler(__ir_handleInterrupt, 0x21);

	if(!ir_trampoline_init(unused))
		return false;

	ir_pic_init();

	return true;
}
