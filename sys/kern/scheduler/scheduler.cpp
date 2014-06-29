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
#include <libc/string.h>
#include <libcpp/algorithm.h>
#include <libcpp/new.h>
#include <machine/interrupts/interrupts.h>
#include <machine/clock/clock.h>
#include <kern/panic.h>
#include "scheduler.h"

namespace Core
{
	void KernelTask()
	{
		kprintf("Hello World\n");

		while(1)
		{}
	}

	void IdleTask()
	{
		kprintf("idle task on CPU %i\n", Sys::CPU::GetCurrentCPU()->GetID());

		while(1)
			Sys::CPUHalt();
	}

	namespace Scheduler
	{
		struct CPUProxy
		{
			CPUProxy(Sys::CPU *tcpu) : 
				cpu(tcpu),
				idleThread(nullptr),
				activeThread(nullptr),
				active(false),
				firstRun(false),
				lock(SPINLOCK_INIT)
			{}

			Sys::CPU *cpu;
			Thread *idleThread;
			Thread *activeThread;
			spinlock_t lock;
			bool active;
			bool firstRun;
		};

		static size_t _proxyCPUCount = 0;
		static CPUProxy *_proxyCPUs = nullptr;
		static spinlock_t _threadLock = SPINLOCK_INIT;

		static Task *_kernelTask = nullptr;
		static Task *_idleTask   = nullptr;

		cpp::queue<Thread> _activeThreads;

		// --------------------
		// MARK: -
		// MARK: Lookup
		// --------------------

		Task *GetActiveTask()
		{
			Thread *thread = GetActiveThread();
			return thread->GetTask();
		}
		Task *GetActiveTask(Sys::CPU *cpu)
		{
			Thread *thread = GetActiveThread(cpu);
			return thread->GetTask();
		}

		Thread *GetActiveThread()
		{
			Sys::CPU *cpu = Sys::CPU::GetCurrentCPU();
			CPUProxy *proxy = &_proxyCPUs[cpu->GetID()];

			return proxy->activeThread;
		}
		Thread *GetActiveThread(Sys::CPU *cpu)
		{
			if(cpu == Sys::CPU::GetCurrentCPU())
				return GetActiveThread();

			CPUProxy *proxy = &_proxyCPUs[cpu->GetID()];

			spinlock_lock(&proxy->lock);
			Thread *thread = proxy->activeThread;
			spinlock_unlock(&proxy->lock);

			return thread;
		}

		// --------------------
		// MARK: -
		// MARK: Thread handling
		// --------------------

		void AddThread(Thread *thread)
		{
			spinlock_lock(&_threadLock);
			_activeThreads.insert_back(thread->GetSchedulerEntry());
			spinlock_unlock(&_threadLock);
		}

		// --------------------
		// MARK: -
		// MARK: Scheduling
		// --------------------

		uint32_t RescheduleOnCPU(uint32_t esp, Sys::CPU *cpu)
		{
			CPUProxy *proxy = &_proxyCPUs[cpu->GetID()];
			Thread *thread  = proxy->activeThread;

			thread->SetQuantum(0);

			return ScheduleOnCPU(esp, cpu);
		}

		uint32_t ScheduleOnCPU(uint32_t esp, Sys::CPU *cpu)
		{
			CPUProxy *proxy = &_proxyCPUs[cpu->GetID()];

			if(__expect_false(!proxy->active) || __expect_false(!spinlock_try_lock(&proxy->lock)))
				return esp;

			Thread *thread = proxy->activeThread;
			thread->Lock();

			if(__expect_true(!proxy->firstRun))
				thread->SetESP(esp);

			if(thread->ReduceQuantum() <= 0 || !thread->IsSchedulable(cpu))
			{
				Thread *original = thread;
				cpp::queue<Thread>::entry *entry = &thread->GetSchedulerEntry();
				bool found = false;

				thread->Unlock();
				do {

					spinlock_lock(&_threadLock);

					entry = entry->get_next();
					if(!entry)
						entry = _activeThreads.get_head();

					spinlock_unlock(&_threadLock);

					thread = entry->get();
					thread->Lock();

					if(thread->IsSchedulable(cpu))
					{
						found = true;
						break;
					}

					thread->Unlock();

				} while(thread != original);

				if(!thread)
				{
					thread = proxy->idleThread;
					thread->Lock();
				}


				proxy->activeThread = thread;

				thread->SetRunningCPU(cpu);
				thread->SetQuantum(5);
				thread->Unlock();

				if(thread != original)
				{
					original->Lock();
					original->SetRunningCPU(nullptr);
					original->Unlock();
				}
			}
			else
			{
				thread->Unlock();
			}

			proxy->firstRun = false;
			spinlock_unlock(&proxy->lock);

			Task *task = thread->GetTask();
			Sys::Trampoline *data = cpu->GetTrampoline();

			data->pageDirectory = task->GetDirectory()->GetPhysicalDirectory();
			data->tss.esp0      = thread->GetESP() + sizeof(Sys::CPUState);

			return thread->GetESP();
		}

		void ActivateCPU(Sys::CPU *cpu)
		{
			CPUProxy *proxy = &_proxyCPUs[cpu->GetID()];
			assert(proxy && proxy->active == false);

			proxy->active    = true;
			proxy->firstRun = true;

			proxy->activeThread = proxy->idleThread;
			proxy->activeThread->SetQuantum(1);
		}

		// --------------------
		// MARK: -
		// MARK: Misc & Initialization
		// --------------------

		uint32_t ActivateCPUFromIPI(uint32_t esp, Sys::CPU *cpu)
		{
			Sys::APIC::SetTimer(Sys::Clock::GetTimerDivisor(), Sys::APIC::TimerMode::Periodic, Sys::Clock::GetTimerCount());
			Sys::APIC::ArmTimer(Sys::Clock::GetTimerCount());

			ActivateCPU(cpu);
			return ScheduleOnCPU(esp, cpu);
		}

		kern_return_t InitializeTasks()
		{
			_kernelTask = new Task(Sys::VM::Directory::GetKernelDirectory());
			_kernelTask->AttachThread(nullptr, (Thread::Entry)&KernelTask, 0);

			_idleTask = new Task(Sys::VM::Directory::GetKernelDirectory());
			
			for(size_t i = 0; i < _proxyCPUCount; i ++)
			{
				Thread *thread;
				kern_return_t result;

				if((result = _idleTask->AttachThread(&thread, (Thread::Entry)&IdleTask, 0)) != KERN_SUCCESS)
					return result;
				

				CPUProxy *proxy = _proxyCPUs + i;

				thread->SetRunningCPU(proxy->cpu);
				thread->SetPinnedCPU(proxy->cpu);

				proxy->idleThread   = thread;
				proxy->activeThread = thread;
			}

			return KERN_SUCCESS;
		}
	}

	kern_return_t SchedulerInit()
	{
		size_t count = Sys::CPU::GetCPUCount();
		void *data = kalloc(count * sizeof(Scheduler::CPUProxy));

		if(!data)
			return KERN_NO_MEMORY;

		Scheduler::_proxyCPUCount = count;
		Scheduler::_proxyCPUs     = reinterpret_cast<Scheduler::CPUProxy *>(data);

		for(size_t i = 0; i < Scheduler::_proxyCPUCount; i ++)
		{
			new(&Scheduler::_proxyCPUs[i]) Scheduler::CPUProxy(Sys::CPU::GetCPUWithID(i));
		}

		kern_return_t result;

		if((result = Scheduler::InitializeTasks()) != KERN_SUCCESS)
			return result;

		Sys::SetInterruptHandler(0x35, &Scheduler::ScheduleOnCPU);
		Sys::SetInterruptHandler(0x3a, &Scheduler::ActivateCPUFromIPI);

		return KERN_SUCCESS;
	}
}
