//
//  task.h
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

#ifndef _TASK_H_
#define _TASK_H_

#include <prefix.h>
#include <libc/stddef.h>
#include <libc/stdint.h>
#include <libcpp/atomic.h>
#include <libcpp/list.h>
#include <libcpp/queue.h>
#include <machine/memory/memory.h>

#include <kern/types.h>
#include <kern/kern_return.h>
#include <kern/spinlock.h>
#include "thread.h"

namespace sd
{
	class scheduler_t;
	
	class task_t : public cpp::list<task_t>::node
	{
	public:
		friend class thread_t;
		friend class scheduler_t;

		enum class state
		{
			waiting,
			running,
			blocked,
			died
		};

		task_t(vm::directory *directory);
		~task_t() override;

		kern_return_t attach_thread(thread_t **outthread, thread_t::entry_t entry, size_t stack);

		pid_t get_pid() const { return _pid; }

	private:
		void attach_thread(thread_t *thread);
		void remove_thread(thread_t *thread);

		std::atomic<int32_t> _tid_counter;
		vm::directory *_directory;

		cpp::queue<thread_t> _threads;
		thread_t *_main_thread;

		spinlock_t _lock;
		pid_t _pid;
		state _state;

		bool _ring3;
	};
}

#endif /* _TASK_H_ */
