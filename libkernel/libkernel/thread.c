//
//  thread.c
//  libkernel
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

#include "thread.h"

extern thread_t __IOPrimitiveThreadCreate(void *entry, void *arg1, void *arg2);
extern thread_t __IOPrimitiveThreadID();
extern void __IOPrimitiveThreadSetName(thread_t id, const char *name);
extern void *__IOPrimitiveThreadAccessArgument(size_t index);
extern void __IOPrimitiveThreadDied();

void __kern_threadEntry()
{
	thread_entry_t entry = (thread_entry_t)__IOPrimitiveThreadAccessArgument(0);
	void *arg = __IOPrimitiveThreadAccessArgument(1);

	entry(arg);
	__IOPrimitiveThreadDied();
}


thread_t kern_threadCreate(thread_entry_t entry, void *arg)
{
	thread_t thread = __IOPrimitiveThreadCreate((void *)__kern_threadEntry, (void *)entry, (void *)arg);
	return thread;
}

thread_t kern_threadCurrent()
{
	return __IOPrimitiveThreadID();
}

void kern_thread_setName(thread_t thread, const char *name)
{
	__IOPrimitiveThreadSetName(thread, name);
}
