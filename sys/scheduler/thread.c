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
#include <system/assert.h>
#include <system/panic.h>
#include <system/syslog.h>
#include <interrupts/trampoline.h>
#include <memory/memory.h>

#include "thread.h"
#include "process.h"

#define THREAD_MAX_TICKS 10
#define THREAD_WANTED_TICKS 4

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

thread_t *thread_createVoid()
{
	thread_t *thread = (thread_t *)halloc(NULL, sizeof(thread_t));
	if(thread)
	{
		// Setup general stuff
		thread->id             = THREAD_NULL;
		thread->died           = false;
		thread->hasBeenRunning = false;
		thread->wasNice        = true;

		// Debugging related
		thread->name    = NULL;
		thread->watched = false;

		// Priority and scheduling stuff
		thread->maxTicks	= THREAD_MAX_TICKS;
		thread->wantedTicks	= THREAD_WANTED_TICKS;
		thread->usedTicks	= 0;

		thread->blocked = 0;
		thread->blocks  = NULL;
		thread->next 	= NULL;

		// Stack stuff
		thread->userStackSize   = 0;
		thread->userStack       = NULL;
		thread->userStackVirt   = NULL;
		thread->kernelStack     = NULL;
		thread->kernelStackVirt = NULL;

		// Sleeping related
		thread->sleeping   = false;
		thread->wakeupCall = 0;
	}

	return thread;
}

thread_t *thread_createKernel(process_t *process, thread_entry_t entry, size_t UNUSED(stackSize), uint32_t argCount, va_list args)
{
	thread_t *thread = thread_createVoid();
	if(thread)
	{
		//size_t stackPages = 1; //MAX(1, stackSize / 4096);
		uint8_t *kernelStack = (uint8_t *)pm_alloc(1);

		if(!kernelStack)
		{
			hfree(NULL, thread);
			return NULL;
		}

		thread->entry   = entry;
		thread->process = process;

		// Create the kernel stack
		thread->kernelStack     = kernelStack;
		thread->kernelStackVirt = (uint8_t *)vm_allocLimit(process->pdirectory, (uintptr_t)kernelStack, THREAD_STACK_LIMIT, 1, VM_FLAGS_KERNEL);

		uint32_t *stack = ((uint32_t *)(thread->kernelStackVirt + VM_PAGE_SIZE)) - argCount;
		memset(thread->kernelStackVirt, 0, 1 * VM_PAGE_SIZE);

		// Push the arguments for the thread on its stack
		thread->arguments = NULL;
		thread->argumentCount = argCount;

		if(argCount > 0)
		{
			thread->arguments = (uintptr_t **)halloc(NULL, argCount * sizeof(uintptr_t *));

			for(uint32_t i=0; i<argCount; i++)
			{
				uintptr_t *val = va_arg(args, uintptr_t *);
				thread->arguments[i] = val;
			}
		}

		// Forge initial kernel stackframe
		*(-- stack) = 0x10; // ss
		*(-- stack) = 0x0; // esp, kernel threads use the TSS
		*(-- stack) = 0x0200; // eflags
		*(-- stack) = 0x8; // cs
		*(-- stack) = (uint32_t)entry; // eip

		// Interrupt number and error code
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;

		// General purpose register
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;

		// Segment registers
		*(-- stack) = 0x10;
		*(-- stack) = 0x10;
		*(-- stack) = 0x10;
		*(-- stack) = 0x10;

		// Update the threads
		thread->esp = (uint32_t)stack;

		// Attach the thread to the process;
		spinlock_lock(&process->threadLock); // Acquire the process' thread lock so we don't end up doing bad things

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

		spinlock_unlock(&process->threadLock);
	}

	return thread;
}

thread_t *thread_createUserland(process_t *process, thread_entry_t entry, size_t UNUSED(stackSize), uint32_t argCount, va_list args)
{
	thread_t *thread = thread_createVoid();
	if(thread)
	{
		size_t stackPages = 1; //MAX(1, stackSize / 4096);

		uint8_t *userStack 	 = (uint8_t *)pm_alloc(1);
		uint8_t *kernelStack = (uint8_t *)pm_alloc(1);

		if(!userStack || !kernelStack)
		{
			if(userStack)
				pm_free((uintptr_t)userStack, 1);
			
			if(kernelStack)
				pm_free((uintptr_t)kernelStack, 1);
			
			hfree(NULL, thread);
			return NULL;
		}

		// Setup general stuff
		thread->entry   = entry;
		thread->process = process;

		// User and kernel stack
		thread->userStack   = userStack;
		thread->kernelStack = kernelStack;

		// Map the user stack
		thread->userStackSize = stackPages;
		thread->userStackVirt = (uint8_t *)vm_allocLimit(process->pdirectory, (uintptr_t)thread->userStack, THREAD_STACK_LIMIT, 1, VM_FLAGS_USERLAND);

		uint32_t *ustack = (uint32_t *)vm_alloc(vm_getKernelDirectory(), (uintptr_t)thread->userStack, 1, VM_FLAGS_KERNEL);
		memset(ustack, 0, 1 * VM_PAGE_SIZE);

		thread->arguments = NULL; // Only for kernel threads

		// Push the arguments for the thread on its stack
		if(argCount > 0)
		{
			uint32_t *vstack = ustack + 1024;
			vstack -= argCount;

			for(uint32_t i=0; i<argCount; i++)
			{
				uint32_t val = va_arg(args, uint32_t);
				*(vstack ++) = val;
			}
		}

		vm_free(vm_getKernelDirectory(), (vm_address_t)ustack, 1);

		// Forge initial kernel stackframe
		thread->kernelStackVirt = (uint8_t *)vm_allocTwoSidedLimit(process->pdirectory, (uintptr_t)kernelStack, THREAD_STACK_LIMIT, 1, VM_FLAGS_KERNEL);

		uint32_t *stack = (uint32_t *)(thread->kernelStackVirt + VM_PAGE_SIZE);

		*(-- stack) = 0x23; // ss
		*(-- stack) = (uint32_t)(thread->userStackVirt + 4092 - (argCount * sizeof(uint32_t))); // esp
		*(-- stack) = 0x0200; // eflags
		*(-- stack) = 0x1B; // cs
		*(-- stack) = (uint32_t)entry; // eip

		// Interrupt number and error code
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;

		// General purpose register
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;

		// Segment registers
		*(-- stack) = 0x23;
		*(-- stack) = 0x23;
		*(-- stack) = 0x23;
		*(-- stack) = 0x23;

		// Update the threads
		thread->esp = ((uint32_t)(thread->kernelStackVirt + VM_PAGE_SIZE)) - sizeof(cpu_state_t);

		// Attach the thread to the process;
		spinlock_lock(&process->threadLock); // Acquire the process' thread lock so we don't end up doing bad things

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

		spinlock_unlock(&process->threadLock);
	}

	return thread;
}

thread_t *thread_create(process_t *process, thread_entry_t entry, size_t stackSize, uint32_t args, ...)
{
	thread_t *thread;

	va_list vlist;
	va_start(vlist, args);

	if(process->ring0)
	{
		thread = thread_createKernel(process, entry, stackSize, args, vlist);
	}
	else
	{
		thread = thread_createUserland(process, entry, stackSize, args, vlist);
	}

	va_end(vlist);
	return thread;
}



thread_t *thread_copy(struct process_s *process, thread_t *source)
{
	thread_t *thread = halloc(NULL,	sizeof(thread_t));
	if(thread)
	{
		uint8_t *userStack 	 = (uint8_t *)pm_alloc(1);
		uint8_t *kernelStack = (uint8_t *)pm_alloc(1);

		if(!userStack || !kernelStack)
		{
			if(userStack)
				pm_free((uintptr_t)userStack, 1);
			
			if(kernelStack)
				pm_free((uintptr_t)kernelStack, 1);
			
			hfree(NULL, thread);
			return NULL;
		}

		// Setup the general stuff
		thread->id 			= THREAD_NULL;
		thread->entry 		= source->entry;
		thread->died 		= false;

		// Priority and scheduling stuff
		thread->maxTicks	= source->maxTicks;
		thread->wantedTicks	= source->wantedTicks;
		thread->usedTicks	= 0;

		thread->blocked = 0;
		thread->blocks  = NULL;
		thread->process = process;
		thread->next 	= NULL;

		// Stack handling
		thread->userStack   = userStack;
		thread->kernelStack = kernelStack;

		// User stack
		thread->userStackVirt = source->userStackVirt;
		thread->userStackSize = source->userStackSize;

		vm_mapPageRange(process->pdirectory, (uintptr_t)userStack, (vm_address_t)thread->userStackVirt, 1, VM_FLAGS_USERLAND);

		void *ustackSource = (void *)vm_alloc(vm_getKernelDirectory(), vm_resolveVirtualAddress(source->process->pdirectory, (vm_address_t)source->userStackVirt), 1, VM_FLAGS_KERNEL);
		void *ustackTarget = (void *)vm_alloc(vm_getKernelDirectory(), (uintptr_t)thread->userStack, 1, VM_FLAGS_KERNEL);

		memcpy(ustackTarget, ustackSource, VM_PAGE_SIZE);

		vm_free(vm_getKernelDirectory(), (vm_address_t)ustackSource, 1);
		vm_free(vm_getKernelDirectory(), (vm_address_t)ustackTarget, 1);

		// Kernel stack
		thread->kernelStackVirt = (uint8_t *)vm_allocTwoSidedLimit(process->pdirectory, (uintptr_t)kernelStack, THREAD_STACK_LIMIT, 1, VM_FLAGS_KERNEL);

		memcpy(thread->kernelStackVirt, source->kernelStackVirt, VM_PAGE_SIZE);

		size_t offset = ((uintptr_t)thread->kernelStackVirt) - ((uintptr_t)source->kernelStackVirt);
		thread->esp = source->esp + offset;

		// Attach the thread to the process;
		spinlock_lock(&process->threadLock); // Acquire the process' thread lock so we don't end up doing bad things

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

		spinlock_unlock(&process->threadLock);
	}

	return thread;
}



void thread_destroy(struct thread_s *thread)
{
	process_t *process = thread->process;
	thread_predicateBecameTrue(thread, thread_predicateOnExit);

	if(thread->userStackVirt)
	{
		vm_free(process->pdirectory, (vm_address_t)thread->userStackVirt, thread->userStackSize);
		pm_free((uintptr_t)thread->userStack, thread->userStackSize);
	}
	
	if(thread->arguments)
	{
		hfree(NULL, thread->arguments);
		thread->arguments = NULL;
	}

	if(thread->name)
	{
		hfree(NULL, (void *)thread->name);
		thread->name = NULL;
	}

	vm_free(process->pdirectory, (vm_address_t)thread->kernelStackVirt, 1);
	vm_free(vm_getKernelDirectory(), (vm_address_t)thread->kernelStackVirt, 1);
	
	pm_free((uintptr_t)thread->kernelStack, 1);

	hfree(NULL, thread);
}

void thread_setPriority(thread_t *thread, int priorityy)
{
	int result = MIN(thread->maxTicks, MAX(1, priorityy));
	thread->wantedTicks = result;
}

thread_t *thread_getWithID(uint32_t id)
{
	process_t *process = process_getCurrentProcess();
	if(process)
	{
		thread_t *thread = process->mainThread;
		while(thread)
		{
			if(thread->id == id)
				return thread;
				
			thread = thread->next;
		}
	}
	
	return NULL;
}


void thread_predicateBecameTrue(struct thread_s *thread, thread_predicate_t predicate)
{
	thread_block_t *block = thread->blocks;
	thread_block_t *previous = NULL;
	while(block)
	{
		if(block->predicate == predicate)
		{
			thread_block_t *temp = block;
			block->thread->blocked --;
			
			if(!previous)
			{
				thread->blocks = block->next;
				block = block->next;
			}
			else
			{
				previous->next = block->next;
				block = block->next;
			}
			
			hfree(NULL, temp);
			continue;
		}
		
		previous = block;
		block = block->next;
	}
}

void thread_attachPredicate(struct thread_s *thread, struct thread_s *blockThread, thread_predicate_t predicate)
{
	assert(thread);
	assert(blockThread);
	
	thread_block_t *block = halloc(NULL, sizeof(thread_block_t));
	if(block)
	{
		block->predicate 	= predicate;
		block->thread 		= blockThread;
		block->next			= thread->blocks;
		
		thread->blocks = block;
		blockThread->blocked ++;
	}
}

void thread_setName(struct thread_s *thread, const char *name)
{
	if(thread->name)
	{
		hfree(NULL, (void *)thread->name);
		thread->name = NULL;
	}

	if(name)
	{
		size_t length = strlen(name) + 1;
		char *copy = halloc(NULL, length);

		if(copy)
		{
			strcpy(copy, name);
			thread->name = (const char *)copy;
		}
	}
}

void thread_join(uint32_t id)
{
	thread_t *thread  = thread_getWithID(id);
	thread_t *cthread = thread_getCurrentThread();
	
	if(thread)
		thread_attachPredicate(thread, cthread, thread_predicateOnExit);
}

void thread_sleep(thread_t *thread, uint64_t time)
{
	if(!thread->sleeping)
	{
		thread->wakeupCall = time_getTimestamp();
		thread->blocked ++;
	}

	thread->wakeupCall += time;
	thread->sleeping = true;
}

void thread_wakeup(thread_t *thread)
{
	if(thread->blocked)
	{
		thread->sleeping = false;
		thread->wakeupCall = 0;
		thread->blocked --;
	}
}
