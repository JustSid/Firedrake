//
//  process.c
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
#include <system/assert.h>
#include <syscall/scmmap.h>
#include <libc/string.h>
#include <interrupts/trampoline.h>
#include "process.h"
#include "scheduler.h"

extern void _process_setFirstProcess(process_t *process); // Scheduler.c

uint32_t _process_getUniqueID()
{
	static uint32_t uid = 0;
	return uid ++;
}

// If insert = true, the scheduler must be locked until the thread list is populated!
// otherwise the scheduler doesn't need to be locked
process_t *process_createVoid(bool insert)
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

		process->mappings = list_create(sizeof(mmap_description_t));

		process->threadLock = SPINLOCK_INIT;
		process->mainThread = NULL;
		process->next       = NULL;

		if(insert)
		{
			if(parent)
			{
				process->next = parent->next;
				parent->next  = process;
			}
			else
				_process_setFirstProcess(process);
		}
	}

	return process;
}



process_t *process_createWithFile(const char *name)
{
	spinlock_lock(&_sd_lock);

	process_t *process = process_createVoid(true);
	if(process)
	{
		process->pdirectory = vm_createDirectory();
		process->image 		= dy_executableCreateWithFile(process->pdirectory, name);

		thread_create(process, (thread_entry_t)process->image->entry, 4 * 4096, 0);
	}

	spinlock_unlock(&_sd_lock);
	return process;
}

process_t *process_fork(process_t *parent)
{
	assert(parent);

	process_t *child = process_createVoid(false);
	if(child)
	{
		child->pdirectory = vm_createDirectory();
		child->image = dy_exectuableCopy(child->pdirectory, parent->image);
		child->ring0 = parent->ring0;

		thread_copy(child, parent->scheduledThread);

		// Copy the memory mappings right here
		// Todo: Implement copy-on-write someday because thats going to be a performance bottleneck once processes actually start to copy things around...
		mmap_copyMappings(child, parent);

		// Add the child into the process list
		spinlock_lock(&_sd_lock);

		child->next  = parent->next;
		parent->next = child;

		spinlock_unlock(&_sd_lock);
	}

	return child;
}

process_t *process_createKernel()
{
	spinlock_lock(&_sd_lock);

	process_t *process = process_createVoid(true);
	if(process)
	{		
		process->pdirectory = vm_getKernelDirectory();
		process->ring0      = true;

		thread_create(process, NULL, 1 * 4096, 0);
	}

	spinlock_unlock(&_sd_lock);
	return process;
}

process_t *process_getParent()
{
	process_t *process = process_getCurrentProcess();
	return process->pprocess;
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
		dy_executableRelease(process->image);

	// Remove all mappings
	list_base_t *entry = list_first(process->mappings);
	while(entry)
	{
		mmap_description_t *description = (mmap_description_t *)entry;

		size_t pages = pageCount(description->length);

		vm_free(process->pdirectory, description->vaddress, pages);
		pm_free(description->paddress, pages);

		entry = entry->next;
	}

	list_destroy(process->mappings);
	hfree(NULL, process);
}
