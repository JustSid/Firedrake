//
//  interrupts.cpp
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

#include <machine/port.h>
#include <machine/cpu.h>
#include <machine/debug.h>
#include <kern/kprintf.h>
#include <kern/panic.h>
#include "interrupts.h"
#include "trampoline.h"
#include "apic.h"

#if __i386__
#include "x86/interrupts.h"
#endif

extern "C" uint32_t ir_handle_interrupt(uint32_t esp);
static Sys::InterruptHandler _interrupt_handler[IDT_ENTRIES];

void panicSegfault()
{
	uint32_t address;
	__asm__ volatile("movl %%cr2, %0" : "=r" (address));

	panic("Segfault at address %p", (void *)address);
}

uint32_t ir_handle_interrupt(uint32_t esp)
{
	Sys::CPUState *state = reinterpret_cast<Sys::CPUState *>(esp);
	Sys::CPU *cpu = Sys::CPU::GetCurrentCPU();

	bool needsEOI = true;

	Sys::CPUState *prev = cpu->GetLastState();
	cpu->SetState(state);

	switch(state->interrupt)
	{
		case 0x27:
		case 0x2f:
		case 0x32:
			needsEOI = false; // Spurious interrupts
			break;

		case 0x39:
			// Shutdown CPU IPI
			while(1)
			{
				cli();
				Sys::CPUHalt();
			}

			break;

		case 0x8:
			panic("Double fault!\n");
			break;

		case 0xe:
			panicSegfault();
			break;

		default:
		{
			Sys::InterruptHandler handler = _interrupt_handler[state->interrupt];
			if(handler)
				esp = handler(esp, cpu);

			break;
		}
	}

	cpu->SetState(prev);

	state = reinterpret_cast<Sys::CPUState *>(esp);
	state->fs = cpu->GetID();

	if(needsEOI)
		Sys::APIC::Write(Sys::APIC::Register::EOI, 0);

	return esp;
}

namespace Sys
{
	// --------------------
	// MARK: -
	// MARK: IDT
	// --------------------

	void SetIDTEntry(uint64_t *idt, uint32_t index, uint32_t handler, uint32_t selector, uint32_t flags)
	{
		idt[index] = handler & UINT64_C(0xffff);
		idt[index] |= (selector & UINT64_C(0xffff)) << 16;
		idt[index] |= (flags & UINT64_C(0xff)) << 40;
		idt[index] |= ((handler>> 16) & UINT64_C(0xffff)) << 48;
	}

#define SetIDTInterruptEntry(num, flags) \
	do{ SetIDTEntry(idt, num, (reinterpret_cast<uint32_t>(idt_interrupt_ ## num)) + offset, 0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_PRESENT | flags); } while(0)

#define SetIDTInterruptSet(num, flags) \
		SetIDTInterruptEntry(num ## 0, flags); \
		SetIDTInterruptEntry(num ## 1, flags); \
		SetIDTInterruptEntry(num ## 2, flags); \
		SetIDTInterruptEntry(num ## 3, flags); \
		SetIDTInterruptEntry(num ## 4, flags); \
		SetIDTInterruptEntry(num ## 5, flags); \
		SetIDTInterruptEntry(num ## 6, flags); \
		SetIDTInterruptEntry(num ## 7, flags); \
		SetIDTInterruptEntry(num ## 8, flags); \
		SetIDTInterruptEntry(num ## 9, flags); \
		SetIDTInterruptEntry(num ## a, flags); \
		SetIDTInterruptEntry(num ## b, flags); \
		SetIDTInterruptEntry(num ## c, flags); \
		SetIDTInterruptEntry(num ## d, flags); \
		SetIDTInterruptEntry(num ## e, flags); \
		SetIDTInterruptEntry(num ## f, flags)

	void IDTInit(uint64_t *idt, uint32_t offset)
	{
		// Excpetion-Handler
		SetIDTEntry(idt, 0x0, (reinterpret_cast<uint32_t>(idt_exception_divbyzero)) + offset,             0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0x1, (reinterpret_cast<uint32_t>(idt_exception_debug)) + offset,                 0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0x3, (reinterpret_cast<uint32_t>(idt_exception_breakpoint)) + offset,            0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0x4, (reinterpret_cast<uint32_t>(idt_exception_overflow)) + offset,              0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0x5, (reinterpret_cast<uint32_t>(idt_exception_boundrange)) + offset,            0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0x6, (reinterpret_cast<uint32_t>(idt_exception_opcode)) + offset,                0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0x7, (reinterpret_cast<uint32_t>(idt_exception_devicenotavailable)) + offset,    0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0x8, (reinterpret_cast<uint32_t>(idt_exception_doublefault)) + offset,           0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0x9, (reinterpret_cast<uint32_t>(idt_exception_segmentoverrun)) + offset,        0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0xa, (reinterpret_cast<uint32_t>(idt_exception_invalidtss)) + offset,            0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0xb, (reinterpret_cast<uint32_t>(idt_exception_segmentnotpresent)) + offset,     0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0xc, (reinterpret_cast<uint32_t>(idt_exception_stackfault)) + offset,            0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0xd, (reinterpret_cast<uint32_t>(idt_exception_protectionfault)) + offset,       0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0xe, (reinterpret_cast<uint32_t>(idt_exception_pagefault)) + offset,             0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0x10, (reinterpret_cast<uint32_t>(idt_exception_fpuerror)) + offset,             0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0x11, (reinterpret_cast<uint32_t>(idt_exception_alignment)) + offset,            0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0x12, (reinterpret_cast<uint32_t>(idt_exception_machinecheck)) + offset,         0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		SetIDTEntry(idt, 0x13, (reinterpret_cast<uint32_t>(idt_exception_simd)) + offset,                 0x8, IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_RING0 | IDT_FLAG_PRESENT);
		
		// Interrupts
		SetIDTInterruptEntry(0x02, IDT_FLAG_RING0);
		SetIDTInterruptEntry(0x20, IDT_FLAG_RING0);
		SetIDTInterruptEntry(0x21, IDT_FLAG_RING0);
		SetIDTInterruptEntry(0x22, IDT_FLAG_RING0);
		SetIDTInterruptEntry(0x23, IDT_FLAG_RING0);
		SetIDTInterruptEntry(0x24, IDT_FLAG_RING0);
		SetIDTInterruptEntry(0x25, IDT_FLAG_RING0);
		SetIDTInterruptEntry(0x26, IDT_FLAG_RING0);
		SetIDTInterruptEntry(0x27, IDT_FLAG_RING0);
		SetIDTInterruptEntry(0x28, IDT_FLAG_RING0);
		SetIDTInterruptEntry(0x29, IDT_FLAG_RING0);
		SetIDTInterruptEntry(0x2a, IDT_FLAG_RING0);
		SetIDTInterruptEntry(0x2b, IDT_FLAG_RING0);
		SetIDTInterruptEntry(0x2c, IDT_FLAG_RING0);
		SetIDTInterruptEntry(0x2d, IDT_FLAG_RING0);
		SetIDTInterruptEntry(0x2e, IDT_FLAG_RING0);
		SetIDTInterruptEntry(0x2f, IDT_FLAG_RING0);
		
		SetIDTInterruptSet(0x3, IDT_FLAG_RING0);
		SetIDTInterruptSet(0x4, IDT_FLAG_RING0);
		SetIDTInterruptSet(0x5, IDT_FLAG_RING0);
		SetIDTInterruptSet(0x6, IDT_FLAG_RING0);
		SetIDTInterruptSet(0x7, IDT_FLAG_RING0);
		SetIDTInterruptSet(0x8, IDT_FLAG_RING3);
		SetIDTInterruptSet(0x9, IDT_FLAG_RING0);
		SetIDTInterruptSet(0xa, IDT_FLAG_RING0);
		SetIDTInterruptSet(0xb, IDT_FLAG_RING0);
		SetIDTInterruptSet(0xc, IDT_FLAG_RING0);
		SetIDTInterruptSet(0xd, IDT_FLAG_RING0);
		SetIDTInterruptSet(0xe, IDT_FLAG_RING0);
		SetIDTInterruptSet(0xf, IDT_FLAG_RING0);

		// Reload the IDT
		struct 
		{
			uint16_t limit;
			void *pointer;
		} __attribute__((packed)) idtp;

		idtp.limit = (IDT_ENTRIES * sizeof(uint64_t)) - 1,
		idtp.pointer = idt;

		__asm__ volatile("lidt %0" : : "m" (idtp));
	}

	void SetInterruptHandler(uint8_t vector, InterruptHandler handler)
	{
		_interrupt_handler[vector] = handler;
	}

	void EnableInterrupts()
	{
		CPU *cpu = CPU::GetCurrentCPU();
		cpu->AddFlags(CPU::Flags::InterruptsEnabled);

		sti();
	}
	bool DisableInterrupts()
	{
		cli();

		CPU *cpu = CPU::GetCurrentCPU();
		bool enabled = cpu->GetFlagsSet(CPU::Flags::InterruptsEnabled);
		cpu->RemoveFlags(CPU::Flags::InterruptsEnabled);

		return enabled;
	}

	extern uint32_t HandleWatchpoint(uint32_t esp, Sys::CPU *cpu);

	KernReturn<void> InterruptsInit()
	{
		KernReturn<void> result;

		cli();

		kprintf("apic");
		if((result = APICInit()).IsValid() == false)
			return result;

		kprintf(", trampoline");
		if((result = TrampolineInit()).IsValid() == false)
			return result;

		SetInterruptHandler(1, HandleWatchpoint);

		panic_init();
		EnableInterrupts();

		return ErrorNone;
	}

	KernReturn<void> InterruptsInitAP()
	{
		KernReturn<void> result;

		cli();

		if((result = APICInitCPU()).IsValid() == false)
			return result;

		if((result = TrampolineInitCPU()).IsValid() == false)
			return result;

		EnableInterrupts();
		return ErrorNone;
	}
}
