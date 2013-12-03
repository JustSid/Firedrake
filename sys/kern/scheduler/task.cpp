//
//  task.cpp
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

#include <kern/kprintf.h>
#include <kern/panic.h>
#include "task.h"

namespace sd
{
	static std::atomic<pid_t> _task_pid_counter;

	task_t::task_t(vm::directory *directory) :
		_lock(SPINLOCK_INIT),
		_state(state::waiting),
		_pid(_task_pid_counter.fetch_add(1)),
		_tid_counter(0),
		_main_thread(nullptr),
		_directory(directory)
	{
		_ring3 = false;
		_main_thread = false;
	}

	task_t::~task_t()
	{}


	kern_return_t task_t::attach_thread(thread_t **outthread, thread_t::entry_t entry, size_t stack)
	{
		thread_t *thread = new thread_t(this, entry, stack);
		kern_return_t result = thread->initialize();

		if(result != KERN_SUCCESS)
		{
			delete thread;
			return result;
		}

		if(!_main_thread)
			_main_thread = thread;

		if(outthread)
			*outthread = thread;

		return result;
	}


	void task_t::attach_thread(thread_t *thread)
	{
		spinlock_lock(&_lock);
		_threads.insert_back(thread->_task_entry);
		spinlock_unlock(&_lock);
	}

	void task_t::remove_thread(thread_t *thread)
	{
		spinlock_lock(&_lock);
		_threads.erase(thread->_task_entry);
		spinlock_unlock(&_lock);
	}
}
