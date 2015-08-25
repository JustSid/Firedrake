//
//  scheduler.cpp
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

#include <libc/assert.h>
#include <kern/kprintf.h>
#include <kern/panic.h>
#include <libc/string.h>
#include <libcpp/algorithm.h>
#include <machine/clock/clock.h>
#include <kern/panic.h>

#include "scheduler.h"
#include "smp/smp_scheduler.h"

namespace OS
{
	void KernelTaskMain();

	void IdleTask()
	{
		while(1)
			Sys::CPUHalt();
	}

	static Scheduler *_sharedScheduler;

	Scheduler::Scheduler()
	{
		spinlock_init(&_taskLock);
	}

	Scheduler *Scheduler::GetScheduler()
	{
		return _sharedScheduler;
	}

	Task *Scheduler::GetTaskWithPID(pid_t pid) const
	{
		Task *task = nullptr;

		spinlock_lock(&_taskLock);

		std::intrusive_list<Task>::member *entry = _tasks.head();
		while(entry)
		{
			Task *temp = entry->get();

			if(temp->GetPid() == pid)
			{
				task = temp;
				break;
			}

			entry = entry->next();
		}

		spinlock_unlock(&_taskLock);

		return task;
	}

	void Scheduler::AddTask(Task *task)
	{
		spinlock_lock(&_taskLock);
		_tasks.push_back(task->schedulerEntry);
		task->Retain();
		spinlock_unlock(&_taskLock);
	}
	void Scheduler::RemoveTask(Task *task)
	{
		spinlock_lock(&_taskLock);
		_tasks.erase(task->schedulerEntry);
		task->Release();
		spinlock_unlock(&_taskLock);
	}

	KernReturn<void> Scheduler::InitializeTasks()
	{
		_kernelTask = Task::Alloc()->Init(nullptr);
		_kernelTask->SetName(IO::String::Alloc()->InitWithCString("kernel_task"));
		_kernelTask->AttachThread((Thread::Entry)&KernelTaskMain, Thread::PriorityClass::PriorityClassNormal, 0, nullptr);

		return ErrorNone;
	}

	KernReturn<void> SchedulerInit()
	{
		_sharedScheduler = new SMPScheduler();

		if(!_sharedScheduler)
			return Error(KERN_NO_MEMORY);

		KernReturn<void> result;

		if((result = _sharedScheduler->InitializeTasks()).IsValid() == false)
			return result;

		return ErrorNone;
	}
}
