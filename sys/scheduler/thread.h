//
//  thread.h
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

#ifndef _THREAD_H_
#define _THREAD_H_

#include <prefix.h>
#include <container/list.h>
#include <system/cpu.h>
#include <system/time.h>
#include <system/lock.h>

struct process_s;
struct thread_listener_s;

#define THREAD_NULL UINT32_MAX
#define THREAD_STACK_LIMIT 0xBFFFFFFD

typedef void (*thread_entry_t)();

typedef struct thread_s
{
	uint32_t id;
	thread_entry_t entry;
	spinlock_t lock;

	// Scheduling
	uint8_t maxTicks;
	uint8_t usedTicks;
	uint8_t wantedTicks;
	uint8_t blocks; // The number of resources that block the thread, or 0 if the thread isn't blocked

	bool wasNice; // True if the thread gave CPU time back
	bool died; // True if the thread is dead and can be purged

	// Stack
	size_t userStackPages;
	uint8_t *userStack;
	uint8_t *userStackVirt;

	uint8_t *kernelStack;
	uint8_t *kernelStackVirt;

	uint32_t esp;

	// Arguments passed to the thread.
	// Only valid for ring 0 threads, ring 3 threads get their arguments passed on the stack
	uintptr_t **arguments;
	uint32_t argumentCount;

	// Debugging related
	const char *name;
	bool watched;

	// Notification system
	list_t *listener;

	// TLS
	vm_address_t tlsVirtual;
	size_t tlsPages;

	// Sleeping
	bool sleeping;
	timestamp_t alarm;

	struct process_s *process;
	struct thread_s  *next;
	struct thread_s  *sleepingNext;
} thread_t;

typedef enum
{
	thread_eventDidExit
} thread_event_t;

typedef struct thread_listener_s
{
	thread_event_t event; // The event that is listened for
	thread_t *listener; // The listening thread
	
	bool blocks; // True if the listener is waiting for the event
	bool oneShot;

	void (*callback)(thread_t *source, thread_event_t event);

	struct thread_listener_s *next;
	struct thread_listener_s *prev;
} thread_listener_t;


thread_t *thread_create(struct process_s *process, thread_entry_t entry, size_t stackSize, int *errno, uint32_t args, ...);

thread_t *thread_getCurrentThread();
thread_t *thread_getWithID(uint32_t id);

void thread_attachListener(thread_t *thread, thread_listener_t *listener);
void thread_notify(thread_t *thread, thread_event_t event);

void thread_join(thread_t *thread, thread_t *toJoin, int *errno);
void thread_sleep(thread_t *thread, uint64_t time);
void thread_wakeup(thread_t *thread);

uintptr_t thread_getTLSArea(thread_t *thread, uint32_t pages, int *errno);

void thread_setName(thread_t *thread, const char *name, int *errno);
void thread_setPriority(thread_t *thread, int priority);

#endif /* _THREAD_H_ */
