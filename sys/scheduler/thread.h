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
#include <system/time.h>

struct process_s;
struct thread_block_s;

typedef void (*thread_entry_t)();

typedef struct thread_s
{
	uint32_t id;
	thread_entry_t entry;

	uint8_t maxTicks;
	uint8_t usedTicks;
	uint8_t wantedTicks;
	uint8_t died;

	bool hasBeenRunning;
	bool wasNice; // True if the thread gave back it's resources

	// Thread stacks
	size_t userStackSize;
	uint8_t *userStack;
	uint8_t *userStackVirt;

	uint8_t *kernelStack;
	uint8_t *kernelStackVirt;

	uint32_t esp;

	// Arguments. Kernel threads only
	uintptr_t **arguments;
	uint32_t argumentCount;

	// Debugging related
	const char *name;
	bool watched;

	// Blocking
	struct thread_block_s *blocks;
	uint32_t blocked;

	// Sleeping
	bool sleeping;
	uint64_t wakeupCall;

	struct process_s 	*process;
	struct thread_s 	*next;
	struct thread_s 	*sleepingNext;
} thread_t;

typedef enum
{
	thread_predicateOnExit
} thread_predicate_t;

typedef struct thread_block_s
{
	thread_predicate_t predicate;
	thread_t *thread;
	
	struct thread_block_s *next;
} thread_block_t;


#define THREAD_NULL UINT32_MAX
#define THREAD_STACK_LIMIT 0xBFFFFFFD

thread_t *thread_create(struct process_s *process, thread_entry_t entry, size_t stackSize, uint32_t args, ...);
thread_t *thread_copy(struct process_s *target, thread_t *source); // Creates a copy of the source thread for the target process
thread_t *thread_getCurrentThread(); // Defined in scheduler.c!
thread_t *thread_getCollectableThreads(); // Defined in scheduler.c
thread_t *thread_getWithID(uint32_t id);

void thread_join(uint32_t id); // Don't forget to force a reschedule
void thread_sleep(thread_t *thread, time_t time);
void thread_wakeup(thread_t *thread);
// If this is called using the appropriate syscall, the rescheduling is done automatically

void thread_predicateBecameTrue(struct thread_s *thread, thread_predicate_t predicate);
void thread_attachPredicate(struct thread_s *thread, struct thread_s *blockThread, thread_predicate_t predicate);
void thread_setName(struct thread_s *thread, const char *name);

void thread_destroy(struct thread_s *thread); // Frees the memory of the thread.
// Be careful to not destroy the running thread, no sanity check is performed!

void thread_setPriority(thread_t *thread, int priority);

#endif /* _THREAD_H_ */
