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
#include <libc/stddef.h>
#include <libc/stdint.h>
#include <libcpp/atomic.h>
#include <libcpp/queue.h>
#include <machine/cpu.h>
#include <kern/kern_return.h>
#include <kern/types.h>
#include <kern/spinlock.h>

namespace sd
{
	class task_t;
	class scheduler_t;

	class thread_t
	{
	public:
		friend class scheduler_t;
		friend class task_t;

		typedef uint32_t entry_t;

		thread_t(task_t *task, entry_t entry, size_t stackPages);
		~thread_t();

		void lock();
		void unlock();

		task_t *get_task() const { return _task; }
		tid_t get_tid() const { return _tid; }

		bool is_schedulable(cpu_t *cpu) const;

	private:
		kern_return_t initialize();
		void initialize_kernel_stack(bool ring3);

		task_t *_task;
		tid_t _tid;
		spinlock_t _lock;

		cpu_t *_pinned_cpu;
		cpu_t *_running_cpu;

		cpp::queue<thread_t>::entry _scheduler_entry;
		cpp::queue<thread_t>::entry _task_entry;

		int8_t _quantum;

		size_t _user_stack_pages;
		uint8_t *_user_stack;
		uint8_t *_user_stack_virtual;

		size_t _kernel_stack_pages;
		uint8_t *_kernel_stack;
		uint8_t *_kernel_stack_virtual;

		uint32_t _esp;
		uint32_t _entry;
	};
}

#endif /* _THREAD_H_ */
