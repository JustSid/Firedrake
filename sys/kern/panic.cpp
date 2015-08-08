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

	kputs("\n\033[31mKernel Panic!\033[0m\n");
	kputs("Reason: \"");
	kputs(buffer);
	kputs("\"\n");

	kprintf("Crashing CPU: \033[1;34m%i\033[0m\n", cpu->GetID());

	if(state)
	{
		kprintf("CPU State (interrupt vector \033[1;34m0x%x\033[0m):\n", state->interrupt);
		kprintf("  eax: \033[1;34m%08x\033[0m, ecx: \033[1;34m%08x\033[0m, edx: \033[1;34m%08x\033[0m, ebx: \033[1;34m%08x\033[0m\n", state->eax, state->ecx, state->edx, state->ebx);
		kprintf("  esp: \033[1;34m%08x\033[0m, ebp: \033[1;34m%08x\033[0m, esi: \033[1;34m%08x\033[0m, edi: \033[1;34m%08x\033[0m\n", state->esp, state->ebp, state->esi, state->edi);
		kprintf("  eip: \033[1;34m%08x\033[0m, eflags: \033[1;34m%08x\033[0m.\n", state->eip, state->eflags);
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
