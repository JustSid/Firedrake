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
#include "kernel.h"
#include "syslog.h"
#include "helper.h"

extern void sd_disableScheduler();
extern void syslogd_forceFlush();

void panic_dumpStacktraces()
{
	if(ir_isInsideInterruptHandler())
	{
		cpu_state_t *state = (cpu_state_t *)ir_lastInterruptESP();

		io_library_t *library = NULL;
		uintptr_t resolved = kern_resolveAddress(state->eip);
		const char *name = kern_nameForAddress(resolved, &library);
		char buffer[265];

		if(isCPPName(name))
		{
			demangleCPPName(name, buffer);
			name = (const char *)buffer;
		}

		dbg("CPU State (interrupt vector %i):\n", state->interrupt);
		dbg("  eax: %08x, ecx: %08x, edx: %08x, ebx: %08x\n", state->eax, state->ecx, state->edx, state->ebx);
		dbg("  esp: %08x, ebp: %08x, esi: %08x, edi: %08x\n", state->esp, state->ebp, state->esi, state->edi);
		dbg("  eip: %08x, eflags: %08x.\n", state->eip, state->eflags);
		dbg("  resolved eip: ");
		info("%s (found in %s)\n", name, library ? library->path : "Firedrake");
		dbg("\n");
	}

	process_t *process = process_getCurrentProcess();
	if(!process)
	{
		dbg("Kernel backtrace:\n");
		kern_printBacktraceForThread(NULL, 20);
	}
	else
	{
		thread_t *thread = process->scheduledThread;

		dbg("Kernel backtrace of process %i, thread %i (%s):\n", process->pid, thread->id, thread->name);
		kern_printBacktraceForThread(thread, 15);
	}
}

void panic(const char *format, ...)
{
	ir_disableInterrupts(true); // Disable interrupts (including NMI)
	sd_disableScheduler();

	va_list args;
	va_start(args, format);
	
	// Check the length of the formatted message
	char *buffer = NULL;
	int length = vsnprintf(buffer, 0, format, args);
	
	va_end(args);

	// Allocate enough memory for the message and print it
	// TODO: What if the allocation fails?! We can't panic, but could continue without a message?!
	va_start(args, format);

	size_t pages = pageCount(length);

	buffer = (char *)vm_alloc(vm_getKernelDirectory(), pm_alloc(pages), pages, VM_FLAGS_KERNEL);
	vsnprintf(buffer, length + 1, format, args);

	va_end(args);

	syslogd_forceFlush(); // Make sure that syslogd isn't blocked by anything

	// Print the message
	syslog(LOG_ALERT, "\nKernel Panic!");
	syslog(LOG_INFO, "\nReason: ");
	syslog(LOG_ERROR, "\"%s\"\n\n", buffer);
	

	static bool triedPrintingBacktrace = false;
	if(!triedPrintingBacktrace)
	{
		triedPrintingBacktrace = true;
		panic_dumpStacktraces();
	}
	
	syslog(LOG_ALERT, "\nCPU halted!");
	syslogd_flush(); // Commit the messages

	while(1) 
		__asm__ volatile("cli; hlt"); // And this, dear kids, is how Mr. Kernel died. Alone and without any friends.
}
