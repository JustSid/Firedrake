//
//  scheduler_syscall.cpp
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

#include <os/syscall/syscall.h>
#include <kern/kprintf.h>
#include <libio/core/IONumber.h>
#include <os/waitqueue.h>
#include "scheduler_syscall.h"

namespace OS
{
	KernReturn<uint32_t> Syscall_SchedThreadCreate(Thread *thread, SchedThreadCreateArgs *arguments)
	{
		IO::StrongRef<IO::Array> parameters(IOTransferRef(IO::Array::Alloc()->Init()));

		if(!parameters)
			return Error(KERN_NO_MEMORY);

		IO::StrongRef<IO::Number> param1(IOTransferRef(IO::Number::Alloc()->InitWithUint32(arguments->argument1)));
		IO::StrongRef<IO::Number> param2(IOTransferRef(IO::Number::Alloc()->InitWithUint32(arguments->argument2)));

		if(!param1 || !param2)
			return Error(KERN_NO_MEMORY);

		parameters->AddObject(param1);
		parameters->AddObject(param2);

		Task *task = thread->GetTask();
		KernReturn<Thread *> result = task->AttachThread(static_cast<Thread::Entry>(arguments->entry), Thread::PriorityClassNormal, arguments->stack, parameters);

		if(!result.IsValid())
			return result.GetError();

		return result->GetTid();
	}

	void MarkThreadExit(void *arg)
	{
		Task *task = reinterpret_cast<Task *>(arg);
		task->Release();
	}

	KernReturn<uint32_t> Syscall_SchedThreadExit(Thread *thread, SchedThreadExitArgs *arguments)
	{
		Task *task = thread->GetTask();

		if(task->GetMainThread() == thread)
		{
			Sys::CPU::GetCurrentCPU()->GetWorkQueue()->PushEntry(&MarkThreadExit, task);

			task->Retain(); // Defer the Dealloc() until we are out of the syscall handler to avoid crashing
			task->PronounceDead(arguments->exitCode);
		}

		Scheduler *scheduler = Scheduler::GetScheduler();

		scheduler->BlockThread(thread);
		scheduler->RemoveThread(thread);

		task->MarkThreadExit(thread);

		return 0;
	}

	KernReturn<uint32_t> Syscall_SchedThreadJoin(Thread *thread, SchedThreadJoinArgs *arguments)
	{
		Task *task = thread->GetTask();
		Thread *target = task->GetThreadWithID(arguments->tid);

		if(!target)
			return Error(KERN_INVALID_ARGUMENT);

		WaitThread(thread, target->GetJoinToken());
		return 0;
	}

	KernReturn<uint32_t> Syscall_SchedThreadYield(Thread *thread, __unused void *arguments)
	{
		Scheduler *scheduler = Scheduler::GetScheduler();
		scheduler->YieldThread(thread);

		return 0;
	}
}
