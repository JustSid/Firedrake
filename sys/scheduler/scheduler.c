//
//  scheduler.c
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

#include <libc/assert.h>
#include <system/panic.h>
#include <system/port.h>
#include <system/tss.h>
#include <system/syslog.h>
#include <interrupts/interrupts.h>
#include <interrupts/trampoline.h>
#include <libc/string.h>
#include <syscall/syscall.h>
#include "scheduler.h"

static process_t *_process_firstProcess = NULL;
static process_t *_process_currentProcess = NULL;
static process_t *_sd_deadProcess = NULL;
static thread_t  *_sd_deadThread  = NULL;

spinlock_t _sd_lock = SPINLOCK_INIT;

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
	spinlock_lock(&_sd_lock);

	process_t *process = _sd_deadProcess;
	_sd_deadProcess    = NULL;

	spinlock_unlock(&_sd_lock);
	return process;
}

thread_t *thread_getCollectableThreads()
{
	spinlock_lock(&_sd_lock);

	thread_t *thread = _sd_deadThread;
	_sd_deadThread	 = NULL;

	spinlock_unlock(&_sd_lock);
	return thread;
}

thread_t *thread_getCurrentThread()
{
	return _process_currentProcess->scheduledThread;
}



void _process_setFirstProcess(process_t *process)
{
	_process_firstProcess   = process;
	_process_currentProcess = process;
}

void _process_setDeadProcess(process_t *process)
{
	spinlock_lock(&_sd_lock);
	_sd_deadProcess = process;
	spinlock_unlock(&_sd_lock);
}


// MARK: Scheduler helper
// Assume the scheduler to be locked

static inline process_t *_sd_processPreviousProcess(process_t *process)
{
	process_t *previous = _process_firstProcess;
	while(previous)
	{
		if(previous->next == process)
			return previous;

		previous = previous->next;
	}

	return NULL;
}

static inline thread_t *_sd_threadPreviousThread(thread_t *thread)
{
	process_t *process = thread->process;
	thread_t  *previous = process->mainThread;

	while(previous)
	{
		if(previous->next == thread)
			return previous;

		previous = previous->next;
	}

	return NULL;
}


// MARK: Scheduler

#define sd_processIsSchedulable(process) (!process->died && process->blocks == 0)
#define sd_threadIsSchedulable(thread) (!thread->died && thread->blocks == 0 && thread->usedTicks <= thread->wantedTicks)

static inline process_t *sd_cleanUpPair(process_t *process, thread_t *thread)
{
	if(process->died || thread->died)
	{
		if(process->died || thread == process->mainThread)
		{
			process_t *previous = _sd_processPreviousProcess(process);
			previous->next = process->next;

			process->next = _sd_deadProcess;
			_sd_deadProcess = process;

			process = previous->next ? previous->next : _process_firstProcess;
			thread  = process->scheduledThread;
		}
		else
		if(thread->died)
		{
			thread_t *previous = _sd_threadPreviousThread(thread);
			previous->next = thread->next;

			thread->next = _sd_deadThread;
			_sd_deadThread = thread;

			thread = previous->next ? previous->next : process->mainThread;
			process->scheduledThread = thread;
		}
	}

	return process;
}

static inline thread_t *sd_scheduleThread()
{
	timestamp_t timestamp = time_getTimestamp();

	thread_t *thread;
	process_t *process = _process_currentProcess;

	process->scheduledThread->usedTicks = 0;

	while(1)
	{
		process = process->next ? process->next : _process_firstProcess;
		thread  = process->scheduledThread->next ? process->scheduledThread->next : process->mainThread;

		process->scheduledThread = thread;

		process = sd_cleanUpPair(process, thread);
		thread  = process->scheduledThread;

		if(thread->sleeping && thread->alarm <= timestamp)
			thread_wakeup(thread);

		if(sd_processIsSchedulable(process) && sd_threadIsSchedulable(thread))
			return thread;
	}
}

uint32_t sd_schedule(uint32_t esp)
{
	process_t *process = _process_currentProcess;
	thread_t  *thread  = process->scheduledThread;

	if(esp)
	{
		// Try to obtain the lock, if we can't, we simply return to the current process
		// The reason behind this is that its possible that the currently scheduled thread holds the lock in which case we would deadlock ourself
		bool result = spinlock_tryLock(&_sd_lock);
		if(!result)
			return esp;

		thread->esp = esp;
	}

	if(!sd_processIsSchedulable(process) || !sd_threadIsSchedulable(thread))
	{
		thread  = sd_scheduleThread();
		process = thread->process;
	}

	//if(process->pid != 0)
	//	dbg("Switching to %i:%i\n", process->pid, thread->id);

	_process_currentProcess = process;
	ir_trampoline_map->pagedir  = process->pdirectory;
	ir_trampoline_map->tss.esp0 = thread->esp + sizeof(cpu_state_t);

	thread->wasNice = false;
	thread->usedTicks ++;

	spinlock_unlock(&_sd_lock);
	return thread->esp;
}

uint32_t sd_schedule_kernel(uint32_t esp)
{
	process_t *process = _process_currentProcess;
	thread_t  *thread  = process->scheduledThread;

	thread->wasNice = true;
	thread->usedTicks = thread->wantedTicks;

	return sd_schedule(esp);
}



void sd_yield()
{
	__asm__ volatile("int $0x31");
}

void sd_threadExit()
{
	thread_t *thread = thread_getCurrentThread();
	thread->died = true;

	while(1)
		sd_yield();
}


void sd_lock()
{
	spinlock_lock(&_sd_lock);
}

void sd_unlock()
{
	spinlock_unlock(&_sd_lock);
}


extern process_t *process_forgeInitialProcess();

bool sd_init(__unused void *data)
{
	process_t *process = process_forgeInitialProcess();
	thread_t  *thread = process->mainThread;
	thread_setName(thread, "kerneld", NULL);

	// Prepare everything for the kernel task
	ir_trampoline_map->pagedir  = process->pdirectory;
	ir_trampoline_map->tss.esp0 = thread->esp + sizeof(cpu_state_t);

	// Setup the interrupt handler and enable interrupts now that we have a stack for the interrupt handler
	ir_setInterruptHandler(sd_schedule_kernel, 0x31);
	ir_enableInterrupts(false);

	return (process != NULL);
}
