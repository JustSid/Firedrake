//
//  task.cpp
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

#include <kern/kprintf.h>
#include <kern/panic.h>
#include "scheduler.h"
#include "task.h"

namespace Core
{
	static std::atomic<pid_t> _taskPidCounter;

	namespace Scheduler
	{
		void AddThread(Thread *thread);
	}

	Task::Task(Sys::VM::Directory *directory) :
		_lock(SPINLOCK_INIT),
		_state(State::Waiting),
		_pid(_taskPidCounter.fetch_add(1)),
		_ring3(false),
		_tidCounter(1),
		_mainThread(nullptr),
		_directory(directory)
	{}

	Task::~Task()
	{}


	kern_return_t Task::AttachThread(Thread **outthread, Thread::Entry entry, size_t stack)
	{
		spinlock_lock(&_lock);

		Thread *thread;
		kern_return_t result = Thread::Create(thread, this, entry, stack);

		if(result != KERN_SUCCESS)
		{
			spinlock_unlock(&_lock);
			return result;
		}

		if(!_mainThread)
			_mainThread = thread;

		if(outthread)
			*outthread = thread;

		_threads.insert_back(thread->_taskEntry);
		spinlock_unlock(&_lock);
		
		Scheduler::AddThread(thread);
		return KERN_SUCCESS;
	}

	void Task::RemoveThread(Thread *thread)
	{
		spinlock_lock(&_lock);
		_threads.erase(thread->_taskEntry);
		spinlock_unlock(&_lock);
	}
}
