//
//  kerneld.c
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

#include <memory/memory.h>
#include <system/syslog.h>
#include <system/panic.h>
#include <system/syscall.h>
#include <scheduler/scheduler.h>
#include <libc/string.h>
#include <system/state.h>
#include <libc/stdio.h>

void test()
{
	syscall1(syscall_print, (int32_t)"Hello World!");
}


void kerneld_main()
{
	thread_setPriority(thread_getCurrentThread(), 0.2f);

	// Dummy process
	process_create(test);

	// Enter the default run loop of the kernel daemon
	// Note that the kernel daemon runs in ring 0, so it can do things like 'hlt'
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


		__asm__ volatile("hlt;"); // Idle for the heck of it!
	}
}
