//
//  exceptions.c
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

#include <system/cpu.h>
#include <system/panic.h>
#include <system/syslog.h>
#include <system/kernel.h>
#include <system/helper.h>
#include <libc/stdio.h>
#include <scheduler/scheduler.h>
#include "interrupts.h"

const char *__ir_exception_pageFaultTranslateBit(int bit, uint32_t error);
void __ir_exceptionKillAndDump(cpu_state_t *state, bool forcePanic, const char *message, ...);

uint32_t __ir_handleException(uint32_t esp)
{
	cpu_state_t *state = (cpu_state_t *)esp;

	switch(state->interrupt)
	{
		case 0:
			__ir_exceptionKillAndDump(state, false, "Division by zero");
			return sd_schedule(esp);

		case 2:
			__ir_exceptionKillAndDump(state, true, "NMI");
			return sd_schedule(esp);

		case 6:
			__ir_exceptionKillAndDump(state, false, "Invalid opcode");
			return sd_schedule(esp);

		case 13:
			__ir_exceptionKillAndDump(state, false, "General protection faul. Error code: %x", state->error);
			return sd_schedule(esp);

		case 14:
		{
			// Page Fault
			uint32_t address;
			uint32_t error = state->error;

			__asm__ volatile("mov %%cr2, %0" : "=r" (address)); // Get the virtual address of the page

			const char *reason = __ir_exception_pageFaultTranslateBit(0, error);
			const char *why = __ir_exception_pageFaultTranslateBit(1, error);
			const char *ring = __ir_exception_pageFaultTranslateBit(2, error);

			bool forcePanic = ((error & (1 << 2)) == 0); // If it occured in the kernel, chances are that something is in an invalid state!

			__ir_exceptionKillAndDump(state, forcePanic, "%s (%p) while %s in %s", reason, address, why, ring);
			return sd_schedule(esp);
		}

		default:
			panic("Unhandled exception %i occured!", state->interrupt);
	}
}



void __ir_exceptionKillAndDump(cpu_state_t *state, bool forcePanic, const char *message, ...)
{
	process_t *process = process_getCurrentProcess();
	thread_t *thread = process->scheduledThread;

	char buffer[256];

	va_list args;
	va_start(args, message);
	vsnprintf(buffer, 255, message, args);
	va_end(args);

	if(forcePanic || process->pid == 0)
	{
		panic(buffer);
		return;
	}

	process->died = true;
	
	dbg("Process %i:%i crashed!\nReason: \16\27\"%s\"\16\031.\n", process->pid, thread->id, buffer);
	dbg("  eax: \16\27%08x\16\031, ecx: \16\27%08x\16\031, edx: \16\27%08x\16\031, ebx: \16\27%08x\16\031\n", state->eax, state->ecx, state->edx, state->ebx);
	dbg("  esp: \16\27%08x\16\031, ebp: \16\27%08x\16\031, esi: \16\27%08x\16\031, edi: \16\27%08x\16\031\n", state->esp, state->ebp, state->esi, state->edi);
	dbg("  eip: \16\27%08x\16\031, eflags: \16\27%08x\16\031.\n", state->eip, state->eflags);

	kern_printBacktraceForThread(thread, 15);
}

const char *__ir_exception_pageFaultTranslateBit(int bit, uint32_t error)
{
	switch(bit)
	{
		case 0:
			return (error & (1 << bit)) ? "Protection violation" : "Non-present page";

		case 1:
			return (error & (1 << bit)) ? "writing" : "reading";

		case 2:
			return (error & (1 << bit)) ? "ring 3" : "ring 0";

		default:
			break;
	}

	return "";
}
