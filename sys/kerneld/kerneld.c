//
//  kerneld.c
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

#include <config.h>
#include <memory/memory.h>
#include <system/syslog.h>
#include <system/panic.h>
#include <scheduler/scheduler.h>
#include <interrupts/interrupts.h>
#include <libc/string.h>
#include <libc/stdio.h>
#include <tests/unittests.h>

#include "syslogd.h"
#include "ioglued.h"

extern void sd_disableScheduler();
extern void syslogd_forceFlush();

void kerneld_unitTests() __attribute__((noreturn));

void kerneld_main() __attribute__((noinline, noreturn));
void kerneld_main()
{
	sd_yield(); // Force a reschedule right here, just to make sure that everything is properly initalized at the schedulers end

	// Create system threads
	process_t *self = process_getCurrentProcess();
	thread_create(self, syslogd, 4096, 0);
	thread_create(self, ioglued, 4096, 0);
	thread_create(self, kerneld_unitTests, 4096, 0);

#if CONF_RUNKUNIT == 0
	// Spawn a test process
	//process_createWithFile("hellostatic.bin");
#endif /* CONF_RUNKUNIT */

	// Let's do some work
	while(1)
	{
		// Collect dead processes
		process_t *process = process_getCollectableProcesses();
		while(process)
		{
			process_t *temp = process;
			process = process->next;

			process_destroy(temp);
		}

		// Collect dead threads
		thread_t *thread = thread_getCollectableThreads();
		while(thread)
		{
			thread_t *temp = thread;
			thread = thread->next;
			
			thread_destroy(temp);
		}

		self->mainThread->wasNice = true;
		__asm__ volatile("hlt;"); // Idle for the heck of it!
	}
}

// Unit Tests
void kerneld_unitTests()
{
#if CONF_RUNKUNIT
	runUnitTests();
#if CONF_KUNITEXITATEND
	info("Ran all unit tests, idling now\n");

	ir_disableInterrupts(true);
	sd_disableScheduler();

	syslogd_forceFlush();

	while(1)
		__asm__ volatile("cli; hlt;"); // Stop everything right here and now
#endif /* CONF_KUNITEXITATEND */
#endif /* CONF_RUNKUNIT */

	sd_threadExit();
}
