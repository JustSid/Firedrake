//
//  thread.c
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

#include <errno.h>
#include <libc/math.h>
#include <libc/string.h>
#include <libc/assert.h>
#include <system/panic.h>
#include <system/syslog.h>
#include <interrupts/trampoline.h>
#include <memory/memory.h>

#include "thread.h"
#include "process.h"

#define THREAD_MAX_TICKS 10
#define THREAD_WANTED_TICKS 4

void thread_destroy(thread_t *thread);
void thread_copyTLSArea(thread_t *thread, thread_t *source, vm_page_directory_t pdirectory);

uint32_t _thread_getUniqueID(process_t *process)
{
	uint32_t uid = process->threadCounter ++;
	return uid;
}

thread_t *thread_createVoid(process_t *process, thread_entry_t entry, int *errno)
{
	thread_t *thread = (thread_t *)halloc(NULL, sizeof(thread_t));
	if(thread)
	{
		// Setup general stuff
		thread->id      = THREAD_NULL;
		thread->entry   = entry;
		thread->lock    = SPINLOCK_INIT;
		thread->process = process;

		// Debugging related
		thread->name    = NULL;
		thread->watched = false;

		// Priority and scheduling stuff
		thread->maxTicks    = THREAD_MAX_TICKS;
		thread->wantedTicks = THREAD_WANTED_TICKS;
		thread->usedTicks   = 0;

		thread->died    = false;
		thread->wasNice = true;

		thread->blocks   = 0;
		thread->listener = list_create(sizeof(thread_listener_t), offsetof(thread_listener_t, next), offsetof(thread_listener_t, prev));

		thread->esp  = 0;
		thread->next = NULL;

		// Stack stuff
		thread->userStackPages  = 0;
		thread->userStack       = NULL;
		thread->userStackVirt   = NULL;
		thread->kernelStack     = NULL;
		thread->kernelStackVirt = NULL;

		// TLS
		thread->tlsVirtual = 0;

		// Sleeping related
		thread->sleeping = false;
		thread->alarm    = 0;

		if(!thread->listener)
		{
			hfree(NULL, thread);
			thread = NULL;
		}
	}

	if(!thread && errno)
		*errno = ENOMEM;

	return thread;
}

void thread_attachToProcess(process_t *process, thread_t *thread)
{
	process_lock(process);
	thread->id = _thread_getUniqueID(process);

	if(process->mainThread)
	{
		thread_t *mthread = process->mainThread;

		thread->next  = mthread->next;
		mthread->next = thread;
	}
	else
	{
		process->mainThread 		= thread;
		process->scheduledThread 	= thread;
	}

	process_unlock(process);
}

#define __threadAssert(condition, error) if(!(condition)) { \
			warn("__threadAssert(%s) failed!\n", #condition); \
			thread_destroy(thread); \
			if(errno) \
				*errno = error; \
			return NULL; \
		} \

thread_t *thread_createKernel(process_t *process, thread_entry_t entry, size_t stackSize, uint32_t argCount, va_list args, int *errno)
{
	thread_t *thread = thread_createVoid(process, entry, errno);
	if(thread)
	{
		thread->userStackPages = MIN(32, MAX(5, VM_PAGE_COUNT(stackSize)));

		uint8_t *kernelStack = (uint8_t *)pm_alloc(thread->userStackPages);
		__threadAssert(kernelStack, ENOMEM);

		// Create the kernel stack
		thread->kernelStack     = kernelStack;
		thread->kernelStackVirt = (uint8_t *)vm_allocLimit(process->pdirectory, (uintptr_t)kernelStack, thread->userStackPages, THREAD_STACK_LIMIT, VM_UPPER_LIMIT, VM_FLAGS_KERNEL);

		__threadAssert(thread->kernelStackVirt, ENOMEM);

		uint32_t *stack = (uint32_t *)(thread->kernelStackVirt + (thread->userStackPages * VM_PAGE_SIZE));
		memset(thread->kernelStackVirt, 0, thread->userStackPages * VM_PAGE_SIZE);

		if(argCount > 0)
		{
			uint32_t *temp = stack - argCount;
			stack = temp;

			for(uint32_t i=0; i<argCount; i++)
			{
				uintptr_t val = va_arg(args, uintptr_t);
				*(temp ++) = val;
			}
		}

		// Forge initial kernel stackframe
		*(-- stack) = 0x10; // ss
		*(-- stack) = 0x0;  // esp
		*(-- stack) = 0x0200; // eflags
		*(-- stack) = 0x8;    // cs
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

		thread->esp = (uint32_t)stack;
		thread_attachToProcess(process, thread);
	}

	return thread;
}

thread_t *thread_createUserland(process_t *process, thread_entry_t entry, size_t stackSize, uint32_t argCount, va_list args, int *errno)
{
	thread_t *thread = thread_createVoid(process, entry, errno);
	if(thread)
	{
		size_t stackPages = MIN(64, MAX(24, VM_PAGE_COUNT(stackSize)));

		uint8_t *userStack 	 = (uint8_t *)pm_alloc(stackPages);
		uint8_t *kernelStack = (uint8_t *)pm_alloc(1);

		if(!userStack || !kernelStack)
		{
			if(userStack)
				pm_free((uintptr_t)userStack, stackPages);
			
			if(kernelStack)
				pm_free((uintptr_t)kernelStack, 1);
			
			thread_destroy(thread);
			if(errno)
				*errno = ENOMEM;

			return NULL;
		}

		// User and kernel stack
		thread->userStack   = userStack;
		thread->kernelStack = kernelStack;

		// Map the user stack
		thread->userStackPages = stackPages;
		thread->userStackVirt  = (uint8_t *)vm_allocLimit(process->pdirectory, (uintptr_t)thread->userStack, stackPages, THREAD_STACK_LIMIT, VM_UPPER_LIMIT, VM_FLAGS_USERLAND);

		__threadAssert(thread->userStackVirt, ENOMEM);

		uint32_t *ustack = (uint32_t *)vm_alloc(vm_getKernelDirectory(), (uintptr_t)thread->userStack, stackPages, VM_FLAGS_KERNEL);
		__threadAssert(ustack, ENOMEM);

		memset(ustack, 0, stackPages * VM_PAGE_SIZE);

		// Push the arguments for the thread on its stack
		if(argCount > 0)
		{
			uint32_t *vstack = ustack + (stackPages * 1024);
			vstack -= argCount;

			for(uint32_t i=0; i<argCount; i++)
			{
				uint32_t val = va_arg(args, uint32_t);
				*(vstack ++) = val;
			}
		}

		vm_free(vm_getKernelDirectory(), (vm_address_t)ustack, stackPages);

		// Forge initial kernel stackframe
		thread->kernelStackVirt = (uint8_t *)vm_allocTwoSidedLimit(process->pdirectory, (uintptr_t)kernelStack, 1, THREAD_STACK_LIMIT, VM_UPPER_LIMIT, VM_FLAGS_KERNEL);
		__threadAssert(thread->kernelStackVirt, ENOMEM);

		uint32_t *stack = (uint32_t *)(thread->kernelStackVirt + VM_PAGE_SIZE);

		*(-- stack) = 0x23; // ss
		*(-- stack) = (uint32_t)(thread->userStackVirt + ((thread->userStackPages * VM_PAGE_SIZE) - 4) - (argCount * sizeof(uint32_t))); // esp
		*(-- stack) = 0x0200; // eflags
		*(-- stack) = 0x1B;   // cs
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

		thread->esp = ((uint32_t)(thread->kernelStackVirt + VM_PAGE_SIZE)) - sizeof(cpu_state_t);
		thread_attachToProcess(process, thread);
	}

	return thread;
}

void thread_reentry(thread_t *thread, thread_entry_t entry, uint32_t argCount, ...)
{
	assert(thread->process->ring0 == false);

	// Write the new user stack
	size_t stackPages = thread->userStackPages;
	uint32_t *ustack = (uint32_t *)vm_alloc(vm_getKernelDirectory(), (uintptr_t)thread->userStack, stackPages, VM_FLAGS_KERNEL);
	memset(ustack, 0, stackPages * VM_PAGE_SIZE);

	if(argCount > 0)
	{
		va_list args;
		va_start(args, argCount);

		uint32_t *vstack = ustack + (stackPages * 1024);
		vstack -= argCount;

		for(uint32_t i=0; i<argCount; i++)
		{
			uint32_t val = va_arg(args, uint32_t);
			*(vstack ++) = val;
		}

		va_end(args);
	}

	vm_free(vm_getKernelDirectory(), (vm_address_t)ustack, stackPages);

	// Reset the kernel stack as well
	uint32_t *stack = (uint32_t *)(thread->kernelStackVirt + VM_PAGE_SIZE);

	*(-- stack) = 0x23; // ss
	*(-- stack) = (uint32_t)(thread->userStackVirt + ((stackPages * VM_PAGE_SIZE) - 4)); // esp
	*(-- stack) = 0x0200; // eflags
	*(-- stack) = 0x1B;   // cs
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

	thread->esp = ((uint32_t)(thread->kernelStackVirt + VM_PAGE_SIZE)) - sizeof(cpu_state_t);
}

thread_t *thread_create(process_t *process, thread_entry_t entry, size_t stackSize, int *errno, uint32_t args, ...)
{
	va_list vlist;
	va_start(vlist, args);

	thread_t *thread;
	if(process->ring0)
	{
		thread = thread_createKernel(process, entry, stackSize, args, vlist, errno);
	}
	else
	{
		thread = thread_createUserland(process, entry, stackSize, args, vlist, errno);
	}

	va_end(vlist);
	return thread;
}

thread_t *thread_clone(struct process_s *process, thread_t *source, int *errno)
{
	spinlock_lock(&source->lock);

	thread_t *thread = thread_createVoid(process, source->entry, errno);
	if(thread)
	{
		uint8_t *userStack 	 = (uint8_t *)pm_alloc(source->userStackPages);
		uint8_t *kernelStack = (uint8_t *)pm_alloc(1);

		if(!userStack || !kernelStack)
		{
			if(userStack)
				pm_free((uintptr_t)userStack, source->userStackPages);
			
			if(kernelStack)
				pm_free((uintptr_t)kernelStack, 1);
			
			spinlock_unlock(&source->lock);
			thread_destroy(thread);

			if(errno)
				*errno = ENOMEM;

			return NULL;
		}

		// Stack handling
		thread->userStack   = userStack;
		thread->kernelStack = kernelStack;

		// User stack
		thread->userStackVirt  = source->userStackVirt;
		thread->userStackPages = source->userStackPages;

		vm_mapPageRange(process->pdirectory, (uintptr_t)userStack, (vm_address_t)thread->userStackVirt, thread->userStackPages, VM_FLAGS_USERLAND);

		void *ustackSource = (void *)vm_alloc(vm_getKernelDirectory(), vm_resolveVirtualAddress(source->process->pdirectory, (vm_address_t)source->userStackVirt), thread->userStackPages, VM_FLAGS_KERNEL);
		void *ustackTarget = (void *)vm_alloc(vm_getKernelDirectory(), (uintptr_t)thread->userStack, thread->userStackPages, VM_FLAGS_KERNEL);

		if(!ustackSource || !ustackTarget)
		{
			if(ustackSource)
				vm_free(vm_getKernelDirectory(), (vm_address_t)ustackSource, thread->userStackPages);

			if(ustackTarget)
				vm_free(vm_getKernelDirectory(), (vm_address_t)ustackTarget, thread->userStackPages);

			spinlock_unlock(&source->lock);
			thread_destroy(thread);

			if(errno)
				*errno = ENOMEM;

			return NULL;
		}

		memcpy(ustackTarget, ustackSource, thread->userStackPages * VM_PAGE_SIZE);

		vm_free(vm_getKernelDirectory(), (vm_address_t)ustackSource, thread->userStackPages);
		vm_free(vm_getKernelDirectory(), (vm_address_t)ustackTarget, thread->userStackPages);

		// Kernel stack
		thread->kernelStackVirt = (uint8_t *)vm_allocTwoSidedLimit(process->pdirectory, (uintptr_t)kernelStack, 1, THREAD_STACK_LIMIT, VM_UPPER_LIMIT, VM_FLAGS_KERNEL);
		thread->esp = source->esp + ((uintptr_t)thread->kernelStackVirt) - ((uintptr_t)source->kernelStackVirt);

		if(!thread->kernelStackVirt)
		{
			spinlock_unlock(&source->lock);
			thread_destroy(thread);

			if(errno)
				*errno = ENOMEM;

			return NULL;
		}

		memcpy(thread->kernelStackVirt, source->kernelStackVirt, VM_PAGE_SIZE);

		thread_copyTLSArea(thread, source, process->pdirectory);
		thread_attachToProcess(process, thread);
	}

	spinlock_unlock(&source->lock);
	return thread;
}

void thread_destroy(thread_t *thread)
{
	if(thread->listener)
	{
		thread_notify(thread, thread_eventDidExit);
	}

	process_t *process = thread->process;
	spinlock_lock(&thread->lock);

	if(thread->userStackVirt)
		vm_free(process->pdirectory, (vm_address_t)thread->userStackVirt, thread->userStackPages);

	if(thread->userStack)
		pm_free((uintptr_t)thread->userStack, thread->userStackPages);
	
	if(thread->kernelStackVirt)
	{
		if(process->ring0)
		{
			vm_free(vm_getKernelDirectory(), (vm_address_t)thread->kernelStackVirt, thread->userStackPages);
		}
		else
		{
			vm_free(process->pdirectory, (vm_address_t)thread->kernelStackVirt, 1);
			vm_free(vm_getKernelDirectory(), (vm_address_t)thread->kernelStackVirt, 1);
		}
	}

	if(thread->tlsVirtual)
	{
		uintptr_t physical = vm_resolveVirtualAddress(process->pdirectory, thread->tlsVirtual);
		vm_free(process->pdirectory, thread->tlsVirtual, thread->tlsPages);
		pm_free(physical, thread->tlsPages);
	}

	if(thread->kernelStack)
		pm_free((uintptr_t)thread->kernelStack, 1);

	if(thread->name)
	{
		hfree(NULL, (void *)thread->name);
		thread->name = NULL;
	}

	if(thread->listener)
		list_destroy(thread->listener);

	hfree(NULL, thread);
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


void thread_lock(thread_t *thread)
{
	spinlock_lock(&thread->lock);
}

void thread_unlock(thread_t *thread)
{
	spinlock_unlock(&thread->lock);
}

bool thread_tryLock(thread_t *thread)
{
	return spinlock_tryLock(&thread->lock);
}

void thread_block(thread_t *thread)
{
	thread->blocks ++;
}

void thread_unblock(thread_t *thread)
{
	thread->blocks --;
}

void thread_attachListener(thread_t *thread, thread_listener_t *tlistener)
{
	thread_lock(thread);

	thread_listener_t *listener = list_addBack(thread->listener);
	listener->listener = tlistener->listener;
	listener->event    = tlistener->event;
	listener->blocks   = tlistener->blocks;
	listener->oneShot  = tlistener->oneShot;
	listener->callback = tlistener->callback;

	if(listener->blocks)
	{
		listener->listener->blocks ++;
		listener->oneShot = true;
	}

	thread_unlock(thread);
}

void thread_notify(thread_t *thread, thread_event_t event)
{
	thread_lock(thread);

	thread_listener_t *listener = (thread_listener_t *)list_first(thread->listener);
	while(listener)
	{
		if(listener->event == event)
		{
			if(listener->callback)
				listener->callback(thread, event);

			if(listener->blocks)
				listener->listener->blocks --;

			if(listener->oneShot)
			{
				thread_listener_t *temp = listener;
				listener = listener->next;

				list_remove(thread->listener, temp);
				continue;
			}
		}

		listener = listener->next;
	}

	thread_unlock(thread);
}


void thread_join(thread_t *thread, thread_t *toJoin, __unused int *errno)
{
	thread_listener_t listener;
	listener.listener = thread;
	listener.event    = thread_eventDidExit;
	listener.blocks   = true;
	listener.oneShot  = true;
	listener.callback = NULL;

	thread_attachListener(toJoin, &listener);
}

void thread_sleep(thread_t *thread, uint64_t time)
{
	if(!thread->sleeping)
	{
		thread->alarm = time_getTimestamp();
		thread->sleeping = true;
		thread_block(thread);
	}

	thread->alarm += time;
}

void thread_wakeup(thread_t *thread)
{
	if(thread->sleeping)
	{
		thread->sleeping = false;
		thread->alarm = 0;
		thread_unblock(thread);
	}
}



void thread_copyTLSArea(thread_t *thread, thread_t *source, vm_page_directory_t pdirectory)
{
	thread_block(source);

	process_t *process = source->process;
	uintptr_t pmemory = pm_alloc(source->tlsPages);

	thread->tlsPages = source->tlsPages;
	thread->tlsVirtual = source->tlsVirtual;

	vm_mapPageRange(pdirectory, pmemory, thread->tlsVirtual, thread->tlsPages, VM_FLAGS_USERLAND);

	uintptr_t sourcePmemory = vm_resolveVirtualAddress(process->pdirectory, source->tlsVirtual);
	uint8_t *sourcePointer  = (uint8_t *)vm_alloc(vm_getKernelDirectory(), sourcePmemory, thread->tlsPages, VM_FLAGS_USERLAND);
	uint8_t *targetPointer  = (uint8_t *)vm_alloc(vm_getKernelDirectory(), pmemory, thread->tlsPages, VM_FLAGS_USERLAND);

	memcpy(targetPointer, sourcePointer, thread->tlsPages * VM_PAGE_SIZE);

	vm_free(vm_getKernelDirectory(), (vm_address_t)sourcePointer, thread->tlsPages);
	vm_free(vm_getKernelDirectory(), (vm_address_t)targetPointer, thread->tlsPages);

	thread_unblock(source);
}

uintptr_t thread_getTLSArea(thread_t *thread, uint32_t pages, int *errno)
{
	uint32_t allowedPages = MAX(1, MIN(5, pages));
	bool needsTLSResize = (allowedPages > thread->tlsPages && thread->tlsVirtual != 0);

	if(thread->tlsVirtual && allowedPages <= thread->tlsPages)
		return thread->tlsVirtual;

	process_t *process = thread->process;
	uintptr_t pmemory = pm_alloc(allowedPages);

	if(!pmemory)
	{
		*errno = ENOMEM;
		return -1;
	}

	vm_address_t vmemory = vm_alloc(process->pdirectory, pmemory, allowedPages, VM_FLAGS_USERLAND);
	if(!vmemory)
	{
		pm_free(pmemory, allowedPages);

		*errno = ENOMEM;
		return -1;
	}

	vm_address_t target = vm_alloc(vm_getKernelDirectory(), pmemory, allowedPages, VM_FLAGS_KERNEL);
	memset((void *)target, 0, allowedPages * VM_PAGE_SIZE);

	if(needsTLSResize)
	{
		uintptr_t sourcePmemory = vm_resolveVirtualAddress(process->pdirectory, thread->tlsVirtual);
		vm_address_t source = vm_alloc(vm_getKernelDirectory(), sourcePmemory, thread->tlsPages, VM_FLAGS_USERLAND);

		memcpy((void *)target, (void *)source, thread->tlsPages * VM_PAGE_SIZE);

		vm_free(process->pdirectory, thread->tlsVirtual, thread->tlsPages);
		pm_free(sourcePmemory, thread->tlsPages);
	}

	vm_free(vm_getKernelDirectory(), target, allowedPages);

	thread->tlsVirtual = vmemory;
	thread->tlsPages = allowedPages;
	
	return vmemory;
}


void thread_setName(thread_t *thread, const char *name, int *errno)
{
	thread_lock(thread);

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
		
		if(!copy && errno)
			*errno = ENOMEM;
	}

	thread_unlock(thread);
}

void thread_setPriority(thread_t *thread, int priorityy)
{
	thread_lock(thread);

	int result = MIN(thread->maxTicks, MAX(1, priorityy));
	thread->wantedTicks = result;

	thread_unlock(thread);
}
