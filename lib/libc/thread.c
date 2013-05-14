//
//  thread.c
//  libc
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

#include "sys/syscall.h"
#include "thread.h"

typedef void (*__thread_entry_point_t)(void *);

void __thread_entry(void *tentry, void *arg) __attribute__((noinline)); 
void __thread_entry(void *tentry, void *arg)
{
	__thread_entry_point_t entry = (__thread_entry_point_t)tentry;
	entry(arg);

	syscall(SYS_THREADEXIT);
}

thread_t thread_create(void *entry, void *arg)
{
	thread_t result = syscall(SYS_THREADATTACH, __thread_entry, 4096, entry, arg);
	return result;
}

thread_t thread_self()
{
	return syscall(SYS_THREADSELF);
}

void thread_join(thread_t id)
{
	syscall(SYS_THREADJOIN, id);
}

void yield()
{
	syscall(SYS_THREADSLEEP, 0);
}

void sleep(uint32_t time)
{
	syscall(SYS_THREADSLEEP, time);
}
