//
//  exceptions.c
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

#include <system/cpu.h>
#include <system/panic.h>
#include <system/syslog.h>
#include <system/kernel.h>
#include <scheduler/scheduler.h>
#include "interrupts.h"

const char *__ir_exception_pageFaultTranslateBit(int bit, int set);

uint32_t __ir_handleException(uint32_t esp)
{
	cpu_state_t *state = (cpu_state_t *)esp;

	if(state->interrupt == 1)
	{
		// Debug exception
		uint32_t dr6;
		__asm__ volatile("mov %%dr6, %0" : "=r" (dr6));

		for(int i=0; i<3; i++)
		{
			bool didBreak = (dr6 & (1 << i));
			if(didBreak)
			{
				uintptr_t address = 0x0;
				switch(i)
				{
					case 0:
						__asm__ volatile("mov %%dr0, %0" : "=r" (address));
						break;

					case 1:
						__asm__ volatile("mov %%dr1, %0" : "=r" (address));
						break;

					case 2:
						__asm__ volatile("mov %%dr2, %0" : "=r" (address));
						break;

					case 3:
						__asm__ volatile("mov %%dr3, %0" : "=r" (address));
						break;

					default:
						break;
				}

				warn("Watchpoint #%i triggered. Address: %p\n", i, address);
				kern_printBacktrace(20);
			}
		}


		// We have to clear DR6 ourself
		__asm__ volatile("mov %0, %%dr6" : : "r" (0x0));

		return esp;
	}
	else
	if(state->interrupt == 13)
	{
		process_t *process = process_getCurrentProcess();
		if(process->pid == 0)
		{
			panic("General protection fault. Error code: %p", state->error);
		}
		else
		{
			thread_t *thread = process->scheduledThread;
			process->died = true;

			dbg("General protection fault in %i:%i\n", process->pid, thread->id);
			return sd_schedule(esp);
		}
	}
	else
	if(state->interrupt == 14)
	{
		// Page Fault
		uint32_t address;
		uint32_t error = state->error;

		__asm__ volatile("mov %%cr2, %0" : "=r" (address)); // Get the virtual address of the page

		process_t *process = process_getCurrentProcess();
		if(process->pid == 0)
		{
			panic("Page Fault exception; %s while %s in %s.\nVirtual address: %p. EIP: %p", __ir_exception_pageFaultTranslateBit(1, error & (1 << 0)), // Panic with the type of the error
				__ir_exception_pageFaultTranslateBit(2, error & (1 << 1)), // Why it occured
				__ir_exception_pageFaultTranslateBit(3, error & (1 << 2)), // And in which mode it occured.
				address, state->eip);
		}
		else
		{
			thread_t *thread = process->scheduledThread;
			process->died = true;

			dbg("%s while %s (to/from) address %p, caused by %i:%i. Killing.\n", __ir_exception_pageFaultTranslateBit(1, error & (1 << 0)), __ir_exception_pageFaultTranslateBit(2, error & (1 << 1)), address, process->pid, thread->id);
			return sd_schedule(esp);
		}
	}

	panic("Unhandled exception %i occured!", state->interrupt);
}

const char *__ir_exception_pageFaultTranslateBit(int bit, int set)
{
	switch(bit)
	{
		case 1:
			return set ? "Protection violation" : "Non-present page";

		case 2:
			return set ? "writing" : "reading";

		case 3:
			return set ? "ring 3" : "ring 0";

		default:
			break;
	}

	return "";
}
