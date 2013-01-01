//
//  thread.c
//  libkernel
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

#include "thread.h"
#include "spinlock.h"

extern thread_t __io_threadCreate(void *entry, void *owner, void *arg);
extern thread_t __io_threadID();
extern void __io_threadSetName(thread_t id, const char *name);


thread_t kern_threadCreate(thread_entry_t entry, void *arg)
{
	kern_spinlock_t test = 0;
	kern_spinlock_lock(&test);

	return __io_threadCreate((void *)entry, arg, arg);
}

thread_t kern_threadCurrent()
{
	return __io_threadID();
}

void kern_thread_setName(thread_t thread, const char *name)
{
	__io_threadSetName(thread, name);
}
