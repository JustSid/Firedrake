//
//  process.h
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

#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <types.h>
#include <container/list.h>
#include <system/lock.h>
#include <system/kernel.h>
#include <system/time.h>
#include <memory/memory.h>
#include <loader/loader.h>
#include "thread.h"

typedef struct process_s
{
	vm_page_directory_t pdirectory;

	pid_t pid;
	pid_t parent;

	bool died; // True if the process can be collected by the scheduler.
	bool ring0;

	spinlock_t threadLock; // Must be obtained before changing something on the threads
	thread_t *mainThread; // The main thread, ie. the first spawned thread
	thread_t *scheduledThread; // The thread that is currently scheduled
	uint32_t threadCounter; // +1 for every created thread

	timestamp_t startTime;
	timestamp_t usedTime;

	ld_exectuable_t *image;

	list_t *mappings; // used for mmap() 

	struct process_s *pprocess; // Parent
	struct process_s *next;
} process_t;

#define PROCESS_NULL UINT32_MAX

process_t *process_createWithFile(const char *name, int *errno);
process_t *process_fork(process_t *parent, int *errno);
process_t *process_getWithPid(pid_t pid);

process_t *process_getCurrentProcess();
process_t *process_getParent();

#endif /* _PROCESS_H_ */
