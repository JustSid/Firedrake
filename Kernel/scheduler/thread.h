//
//  thread.h
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

#ifndef _THREAD_H_
#define _THREAD_H_

#include <types.h>
#include <system/cpu.h>

typedef void (*thread_entry_t)();
struct process_t;

typedef struct thread_t
{
	cpu_state_t *state;

	uint32_t id;

	uint8_t *stack;
	uint8_t *kernelStack;
	uint8_t *kernelStackTop;
	thread_entry_t entry;

	uint8_t maxTicks;
	uint8_t usedTicks;
	uint8_t wantedTicks;

	bool died;

	struct process_t 	*process;
	struct thread_t 	*next;
} thread_t;

#define THREAD_NULL UINT32_MAX

thread_t *thread_create(struct process_t *process, size_t stackSize, thread_entry_t entry);
thread_t *thread_getCurrentThread(); // Defined in scheduler.c!
thread_t *thread_getCollectableThreads(); // Defined in scheduler.c

void thread_destroy(struct thread_t *thread); // Frees the memory of the thread.
// Be careful to not destroy the running thread, no sanity check is performed!

void thread_setPriority(thread_t *thread, uint32_t priority);

#endif /* _THREAD_H_ */