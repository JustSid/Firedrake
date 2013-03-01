//
//  process.c
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
#include <memory/memory.h>
#include <system/syslog.h>
#include <libc/assert.h>
#include <syscall/scmmap.h>
#include <libc/string.h>
#include <interrupts/trampoline.h>
#include "process.h"
#include "scheduler.h"

extern void _process_setFirstProcess(process_t *process); // Scheduler.c
extern process_t *process_getFirstProcess();

void thread_destroy(thread_t *thread);
void process_destroy(process_t *process);

pid_t _process_getUniqueID()
{
	static pid_t uid = 0;
	return uid ++;
}

process_t *process_createVoid(int *errno)
{
	process_t *process = (process_t *)halloc(NULL, sizeof(process_t));
	if(process)
	{
		process_t *parent = process_getCurrentProcess();

		process->pid   = _process_getUniqueID();
		process->died  = false;
		process->ring0 = false;

		process->parent 	= parent ? parent->pid : PROCESS_NULL;
		process->pprocess 	= parent;

		process->image       = NULL;
		process->pdirectory  = NULL;

		process->startTime = time_getTimestamp();
		process->usedTime  = 0;

		process->mappings = list_create(sizeof(mmap_description_t), offsetof(mmap_description_t, listNext), offsetof(mmap_description_t, listPrev));

		process->threadLock = SPINLOCK_INIT;
		process->mainThread = NULL;
		process->next       = NULL;

		if(!process->mappings)
		{
			hfree(NULL, process);
			process = NULL;
		}
	}

	if(!process && errno)
		*errno = ENOMEM;

	return process;
}

void process_insert(process_t *process)
{
	spinlock_lock(&_sd_lock);

	process_t *parent = process->pprocess;

	process->next = parent->next;
	parent->next  = process;

	spinlock_unlock(&_sd_lock);
}

#define PROCESS_BAILWITHERROR(p, error) do { \
		process_destroy(p); \
		if(errno) \
			*errno = error; \
		return NULL; \
	} while(0)

process_t *process_createWithFile(const char *name, int *errno)
{
	if(!name)
	{
		if(errno)
			*errno = EINVAL;

		return NULL;
	}

	process_t *process = process_createVoid(errno);
	if(process)
	{
		process->pdirectory = vm_createDirectory();
		if(!process->pdirectory)
			PROCESS_BAILWITHERROR(process, ENOMEM);

		process->image = ld_executableCreateWithFile(process->pdirectory, name);
		if(!process->image)
			PROCESS_BAILWITHERROR(process, ENOMEM);

		thread_t *thread = thread_create(process, (thread_entry_t)process->image->entry, 4 * 4096, errno, 0);
		if(!thread)
		{
			process_destroy(process);
			return NULL;
		}

		process_insert(process);
	}

	return process;
}

extern thread_t *thread_clone(struct process_s *target, thread_t *source, int *errno); 

process_t *process_fork(process_t *parent, int *errno)
{
	process_t *child = process_createVoid(errno);
	if(child)
	{
		child->pdirectory = vm_createDirectory();
		if(!child->pdirectory)
			PROCESS_BAILWITHERROR(child, ENOMEM);

		child->image = ld_exectuableCopy(child->pdirectory, parent->image);
		child->ring0 = parent->ring0;

		thread_t *thread = thread_clone(child, parent->scheduledThread, errno);
		if(!thread)
		{
			process_destroy(child);
			return NULL;
		}

		// Copy the memory mappings right here
		// TODO: Implement copy-on-write someday because thats going to be a performance bottleneck once processes actually start to copy things around...
		mmap_copyMappings(child, parent);
		process_insert(child);
	}

	return child;
}

process_t *process_forgeInitialProcess()
{
	spinlock_lock(&_sd_lock);

	process_t *process = process_createVoid(NULL);
	if(process)
	{		
		process->pdirectory = vm_getKernelDirectory();
		process->ring0      = true;

		thread_create(process, NULL, 4096, NULL, 0);
		_process_setFirstProcess(process);
	}

	spinlock_unlock(&_sd_lock);
	return process;
}

process_t *process_getParent()
{
	process_t *process = process_getCurrentProcess();
	return process->pprocess;
}

process_t *process_getWithPid(pid_t pid)
{
	spinlock_lock(&_sd_lock);

	process_t *process = process_getFirstProcess();
	while(process)
	{
		if(process->pid == pid)
			break;

		process = process->next;
	}

	spinlock_unlock(&_sd_lock);
	return process;
}



void process_destroy(process_t *process)
{
	thread_t *thread = process->mainThread;
	thread_t *temp;
	while(thread)
	{
		// Delete all threads...
		temp = thread;
		thread = thread->next;
		
		thread_destroy(temp);
	}

	if(process->image)
		ld_executableRelease(process->image);

	uint32_t seconds = time_getSeconds(process->usedTime);
	uint32_t millisecs = time_getMilliseconds(process->usedTime);

	dbg("Process %i died, used %i.%03i secs on CPU\n", process->pid, seconds, millisecs);

	// Remove all mappings
	mmap_description_t *description = list_first(process->mappings);
	while(description)
	{
		size_t pages = pageCount(description->length);

		vm_free(process->pdirectory, description->vaddress, pages);
		pm_free(description->paddress, pages);

		description = description->next;
	}

	list_destroy(process->mappings);
	hfree(NULL, process);
}
