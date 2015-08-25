//
//  thread.c
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2014 by Sidney Just
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

#include "thread.h"
#include "syscall.h"
#include "tls.h"

void __thread_entry(void (*entry)(void *), void *argument)
{
	entry(argument);

	__asm__ volatile("movl $0, %%eax\r\n"
		             "int $0x80\r\n" : : "c" (0));

	while(1) {}
}

tid_t thread_create(void (*entry)(void *), void *argument)
{
	return (tid_t)SYSCALL4(SYS_ThreadCreate, &__thread_entry, (void *)entry, argument, 0);
}

tid_t thread_gettid()
{
	tid_t result;
	TLS_GET_CPU_DATA_MEMBER(result, tid);

	return result;
}

void thread_join(tid_t thread)
{
	SYSCALL1(SYS_ThreadJoin, thread);
}

void thread_yield()
{
	SYSCALL0(SYS_ThreadYield);
}
