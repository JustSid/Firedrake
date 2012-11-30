//
//  panic.c
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

#include <libc/stdio.h>
#include <libc/string.h>
#include <interrupts/interrupts.h>
#include <scheduler/scheduler.h>
#include <kerneld/syslogd.h>
#include "panic.h"
#include "cpu.h"
#include "kernel.h"
#include "syslog.h"
#include "helper.h"

extern void sd_disableScheduler();
extern void syslogd_forceFlush();

void panic_dumpCPUState()
{
	static spinlock_t spinlock = SPINLOCK_INIT;
	if(spinlock_tryLock(&spinlock))
	{
		// TODO: Currently it's only possible to grab a useful CPU state when panic was entered through an interrupt,
		// but we could alter the panic function so that it pushes all registers onto the stack before doing anything.
		if(ir_isInsideInterruptHandler())
		{
			cpu_state_t *state = (cpu_state_t *)ir_lastInterruptESP();

			// Resolve the instruction pointer and get the name and the library it belongs to, if needed, demangle the name
			io_library_t *library = NULL;
			const char *name = kern_nameForAddress(kern_resolveAddress(state->eip), &library);

			char buffer[255];

			if(isCPPName(name))
			{
				demangleCPPName(name, buffer);
				name = (const char *)buffer;
			}


			// Subtract the relocation base from the instruction pointer if the instruction comes from a library
			// this makes it slightly easier to debug when looking at the objdump output of the library
			uint32_t eip = library ? state->eip - library->relocBase : state->eip;

			dbg("CPU State (interrupt vector \16\27%i\16\031):\n", state->interrupt);
			dbg("  eax: \16\27%08x\16\031, ecx: \16\27%08x\16\031, edx: \16\27%08x\16\031, ebx: \16\27%08x\16\031\n", state->eax, state->ecx, state->edx, state->ebx);
			dbg("  esp: \16\27%08x\16\031, ebp: \16\27%08x\16\031, esi: \16\27%08x\16\031, edi: \16\27%08x\16\031\n", state->esp, state->ebp, state->esi, state->edi);
			dbg("  eip: \16\27%08x\16\031, eflags: \16\27%08x\16\031.\n", eip, state->eflags);

			if(strcmp(name, "<null>"))
				dbg("  resolved eip: \16\27%s\16\031 (found in %s)\n", name, library ? library->name : "Firedrake");

			dbg("\n");
		}
	}
}

void panic_dumpStacktraces()
{
	static spinlock_t spinlock = SPINLOCK_INIT;
	if(spinlock_tryLock(&spinlock))
	{
		process_t *process = process_getCurrentProcess();
		if(!process)
		{
			dbg("Kernel backtrace:\n");
			kern_printBacktraceForThread(NULL, 20);
		}
		else
		{
			thread_t *thread = process->scheduledThread;

			dbg("Kernel backtrace of process %i thread %i (%s):\n", process->pid, thread->id, thread->name);
			kern_printBacktraceForThread(thread, 15);
		}
	}
}



void panic_printReasonOnStack(size_t length, const char *format, va_list args)
{
	char buffer[length];
	vsnprintf(buffer, length, format, args);

	syslog(LOG_ERROR, "\"%s\"\n\n", buffer);
}

void panic_printReason(const char *format, va_list args)
{
	syslog(LOG_ALERT, "\nKernel Panic!");
	syslog(LOG_INFO, "\nReason: ");

	va_list copy;
	va_copy(copy, args);

	char *buffer = NULL;
	int length = vsnprintf(buffer, 0, format, args);

	if(length < 255)
	{
		panic_printReasonOnStack(length + 1, format, copy);
		return;
	}

	buffer = mm_alloc(vm_getKernelDirectory(), pageCount(length), VM_FLAGS_KERNEL);
	if(!buffer)
	{
		buffer = "Failed to allocate memory for panic reason!";
	}
	else
	{
		vsnprintf(buffer, length + 1, format, copy);
	}

	syslog(LOG_ERROR, "\"%s\"\n\n", format);
}

void panic(const char *format, ...)
{
	// Make sure that no other thread on the system will be scheduled again
	ir_disableInterrupts(true);
	sd_disableScheduler();

	// Make sure that syslogd isn't locked
	syslogd_forceFlush();
	syslogd_setLogLevel(LOG_DEBUG);

	// Print the panic reasion
	va_list args;
	va_start(args, format);
	
	panic_printReason(format, args);
	
	va_end(args);

	// Print some information about the systems state
	panic_dumpCPUState();
	panic_dumpStacktraces();
	
	syslog(LOG_ALERT, "\nCPU halted!");

	while(1) 
		__asm__ volatile("cli; hlt"); // And this, dear kids, is how Mr. Kernel died. Alone and without any friends.
}
