//
//  thread.c
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

#include <libc/math.h>
#include <libc/string.h>
#include <system/panic.h>
#include <memory/memory.h>
#include <system/syslog.h>

#include "thread.h"
#include "process.h"

#define THREAD_MIN_STACK 4096
#define THREAD_MAX_TICKS 10
#define THREAD_WANTED_TICKS 4

void _thread_entryStub()
{
	thread_t *thread = thread_getCurrentThread();
	if(thread)
	{
		thread->entry();
		thread->died = true;

		while(1){}
	}

	panic("Inside _thread_entryStub() without a scheduled thread!");
}

uint32_t _thread_getUniqueID(process_t *process)
{
	uint32_t uid = 0;
	
	bool found;
	do {
		found = true;
		thread_t *thread = process->mainThread;
		
		while(thread)
		{
			if(thread->id == uid)
			{
				found = false;
				break;
			}
			
			thread = thread->next;
		}
		
		if(!found)
			uid ++;
		
	} while(!found);
	
	return uid;
}

thread_t *thread_create(struct process_t *process, size_t stackSize, thread_entry_t entry)
{
	stackSize = MAX(stackSize, THREAD_MIN_STACK);

	thread_t *thread = (thread_t *)kalloc(sizeof(thread_t));
	if(thread)
	{
		uint8_t *userStack = kalloc(stackSize);
		uint8_t *kernelStack = kalloc(THREAD_MIN_STACK);

		if(!userStack || !kernelStack)
		{
			if(userStack)
				kfree(userStack);
			
			if(kernelStack)
				kfree(kernelStack);
			
			kfree(thread);
			return NULL;
		}

		thread->id 			= THREAD_NULL;
		thread->stack 		= userStack;
		thread->kernelStack = kernelStack;
		thread->kernelStackTop = (uint8_t *)(thread->kernelStack + THREAD_MIN_STACK);

		thread->entry 		= entry;
		thread->died 		= false;

		// Priority and scheduling stuff
		thread->maxTicks	= THREAD_MAX_TICKS;
		thread->wantedTicks	= THREAD_WANTED_TICKS;
		thread->usedTicks	= 0;

		thread->process = process;
		thread->next 	= NULL;

		// Craft the initial CPU state
		cpu_state_t state;
		state.eax = 0;
		state.ebx = 0;
		state.ecx = 0;
		state.edx = 0;
		state.esi = 0;
		state.edi = 0;
		state.ebp = 0;
		
		state.eip = (uint32_t)_thread_entryStub;
		state.esp = (uint32_t)userStack + stackSize;
		
		state.cs = 0x18 | 0x03;
		state.ss = 0x20 | 0x03;
		
		state.eflags  = 0x200;

		// Attach the CPU state to the kernel stack
		thread->state = (cpu_state_t *)(thread->kernelStackTop - sizeof(cpu_state_t));
		memcpy(thread->state, &state, sizeof(cpu_state_t));


		// Attach the thread to the process;
		spinlock_lock(&process->lock);

		thread->id = _thread_getUniqueID(process);
		if(process->mainThread)
		{
			thread_t *mthread = process->mainThread;

			// Attach the new thread next to the main thread
			thread->next  = mthread->next;
			mthread->next = thread;
		}
		else
		{
			process->mainThread 		= thread;
			process->scheduledThread 	= thread;
		}

		spinlock_unlock(&process->lock);
	}

	return thread;
}

void thread_destroy(struct thread_t *thread)
{
	kfree(thread->stack);
	kfree(thread->kernelStack);
	kfree(thread);
}

void thread_setPriority(thread_t *thread, uint32_t priority)
{
}
