//
//  process.h
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

#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <types.h>
#include <container/list.h>
#include <system/lock.h>
#include <memory/memory.h>
#include <dylink/dylink.h>
#include "thread.h"

typedef struct
{
	list_base_t base;

	vm_address_t vaddress;
	uintptr_t paddress;

} mmap_description_t;

typedef struct process_s
{
	vm_page_directory_t pdirectory;

	uint32_t pid;
	uint32_t parent;

	bool died; // True if the process can be collected by the scheduler.
	bool ring0;

	spinlock_t threadLock; // Must be obtained before changing something on the threads
	thread_t *mainThread; // The main thread, ie. the first spawned thread
	thread_t *scheduledThread; // The thread that is currently scheduled

	dy_exectuable_t *image;

	list_t *mappings; // used for mmap() 
	spinlock_t mmapLock; // Must be obtained before doing something with the mmapings list

	struct process_s *pprocess; // Parent
	struct process_s *next;
} process_t;

#define PROCESS_NULL UINT32_MAX

process_t *process_createWithFile(const char *name);
process_t *process_createKernel();

process_t *process_getCurrentProcess(); // Defined in scheduler.c!
process_t *process_getFirstProcess(); // Defined in scheduler.c, returns a process that can be used to iterate through the process list
process_t *process_getCollectableProcesses(); // Scheduler.c too
process_t *process_getParent();

void process_destroy(process_t *process); // Frees all memory used by the process

#endif /* _PROCESS_H_ */
