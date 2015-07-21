//
//  panic.cpp
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

#include <machine/cpu.h>
#include <machine/interrupts/interrupts.h>
#include <libc/stdarg.h>
#include <libc/stdio.h>
#include <libc/backtrace.h>
#include "panic.h"
#include "kprintf.h"

static char _panicScribbleArea[4096];

static bool _panic_initialized = false;

void panic_die(const char *buffer)
{
	Sys::CPU *cpu = Sys::CPU::GetCurrentCPU();
	Sys::CPUState *state = cpu->GetLastState();

	kputs("\n\16\24Kernel Panic!\16\27\n");
	kputs("Reason: \"");
	kputs(buffer);
	kputs("\"\n");

	kprintf("Crashing CPU: \16\031%i\16\27\n", cpu->GetID());

	if(state)
	{
		kprintf("CPU State (interrupt vector \16\0310x%x\16\27):\n", state->interrupt);
		kprintf("  eax: \16\031%08x\16\27, ecx: \16\031%08x\16\27, edx: \16\031%08x\16\27, ebx: \16\031%08x\16\27\n", state->eax, state->ecx, state->edx, state->ebx);
		kprintf("  esp: \16\031%08x\16\27, ebp: \16\031%08x\16\27, esi: \16\031%08x\16\27, edi: \16\031%08x\16\27\n", state->esp, state->ebp, state->esi, state->edi);
		kprintf("  eip: \16\031%08x\16\27, eflags: \16\031%08x\16\27.\n", state->eip, state->eflags);
	}

	void *frames[10];
	int num = state ? backtrace_np((void *)state->ebp, frames, 10) : backtrace(frames, 10);

	for(int i = 0; i < num; i ++)
		kprintf("(%2i) %08x\n", (num - 1) - i, reinterpret_cast<uint32_t>(frames[i]));

	if(state)
	{
		kputs("Kernel backtrace:\n");

		int num = backtrace(frames, 10);

		for(int i = 0; i < num; i ++)
			kprintf("(%2i) %08x\n", (num - 1) - i, reinterpret_cast<uint32_t>(frames[i]));
	}

	kputs("CPU halt");
}
void panic(const char *reason, ...)
{
	if(_panic_initialized)
	{
		// Tear down the system properly to avoid other code to be executed
		Sys::DisableInterrupts();
		Sys::APIC::BroadcastIPI(0x39, false);
	}

	va_list args;
	va_start(args, reason);
	
	vsnprintf(_panicScribbleArea, 4096, reason, args);
	panic_die(_panicScribbleArea);

	va_end(args);

	while(1)
	{
		Sys::DisableInterrupts();
		Sys::CPUHalt();
	}
}

void panic_init()
{
	_panic_initialized = true;
}
