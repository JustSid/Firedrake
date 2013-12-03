//
//  scheduler.h
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

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <prefix.h>
#include <kern/kern_return.h>
#include <libcpp/singleton.h>
#include <libcpp/queue.h>
#include <machine/cpu.h>

#include "task.h"
#include "thread.h"

namespace sd
{
	class scheduler_t : public cpp::singleton<scheduler_t>
	{
	public:
		scheduler_t();

		uint32_t schedule_on_cpu(uint32_t esp, cpu_t *cpu);

		void activate_cpu(cpu_t *cpu);
		void deactivate_cpu(cpu_t *cpu);

	private:
		struct cpu_proxy_t
		{
			cpu_proxy_t(cpu_t *tcpu) :
				cpu(tcpu),
				idle_thread(nullptr),
				active_thread(nullptr),
				active(false),
				first_run(true)
			{}

			cpu_t *cpu;
			thread_t *idle_thread;
			thread_t *active_thread;
			bool active;
			bool first_run;
		};

		void create_kernel_task();
		void create_idle_task();

		cpu_proxy_t *_proxy_cpus[CPU_MAX_CPUS];
		size_t _proxy_cpu_count;

		task_t *_kernel_task;
		task_t *_idle_task;

		cpp::queue<thread_t> _active_threads;
		cpp::queue<thread_t> _sleeping_threads;
		cpp::queue<thread_t> _blocked_threads;
	};

	kern_return_t init();
}

#endif
