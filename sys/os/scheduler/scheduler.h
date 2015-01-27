//
//  scheduler.h
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

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <prefix.h>
#include <kern/kern_return.h>
#include <libc/sys/spinlock.h>
#include <libcpp/queue.h>
#include <machine/cpu.h>

#include "task.h"
#include "thread.h"

namespace OS
{
	namespace Scheduler
	{
		Task *GetActiveTask();
		Task *GetActiveTask(Sys::CPU *cpu);
		Task *GetTaskWithPID(pid_t pid);

		Thread *GetActiveThread();
		Thread *GetActiveThread(Sys::CPU *cpu);

		Task *GetKernelTask();

		void ActivateCPU(Sys::CPU *cpu);

		uint32_t RescheduleOnCPU(uint32_t esp, Sys::CPU *cpu);
		uint32_t ScheduleOnCPU(uint32_t esp, Sys::CPU *cpu);
		void ForceReschedule();
	}

	KernReturn<void> SchedulerInit();
}

#endif
