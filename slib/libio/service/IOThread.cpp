//
//  IOThread.cpp
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

#include <libkern.h>
#include "IOThread.h"

namespace IO
{
	IODefineMeta(Thread, Object)

	Thread *Thread::InitWithEntry(Entry entry, void *argument)
	{
		if(!Object::Init())
			return nullptr;

		_entry = entry;
		_argument = argument;

		_cancelled = false;
		_exited = false;

		return this;
	}

	void Thread::Start()
	{
		thread_create(&Thread::__Entry, this);
	}

	void Thread::Cancel()
	{
		_cancelled = true;
	}

	void Thread::WaitForExit()
	{
		while(!_exited.load(std::memory_order_acquire))
			thread_yield();
	}

	void Thread::__Entry(void *argument)
	{
		Thread *thread = reinterpret_cast<Thread *>(argument);
		thread->_entry(thread, thread->_argument);
		thread->_exited = true;
	}
}
