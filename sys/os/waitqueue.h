//
//  waitqueue.h
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2015 by Sidney Just
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

#ifndef _WAITQUEUE_H_
#define _WAITQUEUE_H_

#include <prefix.h>
#include <libc/stdint.h>
#include <kern/kern_return.h>
#include <libio/core/IOFunction.h>

namespace OS
{
	class Thread;
	
	KernReturn<void> Wait(void *channel);
	KernReturn<void> WaitWithCallback(void *channel, IO::Function<void ()> &&callback);
	KernReturn<void> WaitThread(Thread *thread, void *channel);

	void Wakeup(void *channel);
	void WakeupOne(void *channel);

	KernReturn<void> WaitqueueInit();
}

#endif /* _WAITQUEUE_H_ */
