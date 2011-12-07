//
//  scheduler.c
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

#include <system/assert.h>
#include <system/syslog.h>
#include <system/panic.h>
#include <system/port.h>
#include <system/interrupts.h>
#include <system/tss.h>
#include <libc/string.h>

#include "scheduler.h"

static process_t *_process_firstProcess = NULL;
static process_t *_process_currentProcess = NULL;
static process_t *_sd_deadProcess = NULL;
static thread_t  *_sd_deadThread  = NULL;

static bool _sd_schedulerLocked = false;
static bool _sd_state = false;

static struct tss_t *_sd_tss = NULL; // Same as the one used by the interrupt handler, this is just a cached value...

process_t *process_getCurrentProcess()
{
	return 	_process_currentProcess;
}

process_t *process_getFirstProcess()
{
	return _process_firstProcess;
}

process_t *process_getCollectableProcesses()
{
	_sd_schedulerLocked = true;

	process_t *process 	= _sd_deadProcess;
	_sd_deadProcess 	= NULL;

	_sd_schedulerLocked = false;
	return process;
}

thread_t *thread_getCollectableThreads()
{
	_sd_schedulerLocked = true;

	thread_t *thread 	= _sd_deadThread;
	_sd_deadThread		= NULL;

	_sd_schedulerLocked = false;
	return thread;
}

thread_t *thread_getCurrentThread()
{
	if(_process_currentProcess)
		return _process_currentProcess->scheduledThread;
	
	return NULL;
}

void _process_setFirstProcess(process_t *process)
{
	_process_firstProcess = process;
	_process_currentProcess = process;
}

bool sd_state()
{
	return _sd_state;
}


// MARK: Scheduling
static inline thread_t *_sd_scheduleableThreadForProcess(process_t *process)
{
	thread_t *mthread = process->scheduledThread;
	thread_t *thread = mthread;

	while(thread)
	{
		if(thread->usedTicks == 0)
		{
			return thread;
		}

		thread = thread->next ? thread->next : process->mainThread;

		if(thread == mthread)
			return NULL;
	}

	return thread;
}

cpu_state_t *_sd_schedule(cpu_state_t *state)
{
	if(_sd_schedulerLocked)
	{
		outb(0x20, 0x20);
		return state;
	}

	assert(_process_currentProcess);

	process_t 	*process = _process_currentProcess;
	thread_t 	*thread  = process->scheduledThread;

	if(state)
		memcpy(thread->state, state, sizeof(cpu_state_t));


	if(thread->died)
	{
		if(thread == process->mainThread)
		{
			_process_currentProcess = process->next ? process->next : _process_firstProcess;

			process_t *previous = _process_firstProcess;
			while(previous)
			{
				if(previous->next == process)
					break;

				previous = previous->next;
			}

			previous->next = process->next;

			// Insert the process into the collectable list
			process->next = _sd_deadProcess;
			_sd_deadProcess = process;

			return _sd_schedule(NULL);
		}


		thread_t *previous = process->mainThread;
		while(previous)
		{
			if(previous->next == thread)
				break;

			previous = previous->next;
		}

		thread_t *next;
		previous->next = thread->next;

		next = thread->next ? thread->next : process->mainThread;
		process->scheduledThread = next;

		thread->next = _sd_deadThread;
		_sd_deadThread = thread;

		return _sd_schedule(NULL);
	}


	// Schedule
	thread->usedTicks ++;
	if(thread->usedTicks >= thread->wantedTicks)
	{
		thread->usedTicks = 0;
		process->scheduledThread = thread->next ? thread->next : process->mainThread;

		process = process->next ? process->next : _process_firstProcess;
	}
	
	// Update the state
	_process_currentProcess = process;
	thread = process->scheduledThread;
	_sd_state = true;

	// Prepare the TSS
	_sd_tss->cr3  = (uint32_t)process->context;
	_sd_tss->esp0 = (uint32_t)process->scheduledThread->kernelStackTop;

	outb(0x20, 0x20); // EOI
	return thread->state;
}

// MARK: Init
extern void kerneld_main();

bool sd_init(void *ingored)
{
	process_t *process = process_create(kerneld_main);
	_sd_tss = ir_getTSS();

	ir_setInterruptHandler(_sd_schedule, 0x20);

	return (process != NULL);
}
