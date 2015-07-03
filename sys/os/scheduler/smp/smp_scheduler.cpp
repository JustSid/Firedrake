//
//  smp_scheduler.cpp
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

#include <libcpp/vector.h>
#include <libc/sys/spinlock.h>
#include <libcpp/new.h>
#include <libcpp/atomic.h>
#include <kern/panic.h>
#include <kern/kprintf.h>
#include <machine/clock/clock.h>
#include <machine/interrupts/interrupts.h>
#include "smp_scheduler.h"

namespace OS
{
	// ---------------
	// CPU Scheduler
	// Responsible for doing scheduling decisions for one single CPU
	// ---------------

	class SMPScheduler::CPUScheduler
	{
	public:
		friend class SMPScheduler;

		CPUScheduler(Sys::CPU *cpu) :
			_cpu(cpu),
			_firstRun(true),
			_hasForcedDown(false),
			_activeThread(nullptr),
			_nextThread(nullptr),
			_idleThread(nullptr),
			_needsReschedule(false),
			_enabled(true)
		{
			spinlock_init(&_internalLock);
			spinlock_init(&_commandLock);
		}
		

		uint32_t Schedule(uint32_t esp)
		{
			Thread *thread = _nextThread;

			if(__expect_true(!_firstRun))
				thread->SetESP(esp);

			if(__expect_false(_enabled.load(std::memory_order_acquire) == false))
				return esp;

			SchedulingData *data = thread->GetSchedulingData<SchedulingData>();
			data->usage ++;

			// Decay CPU usage roughly every 500ms
			_time += Sys::Clock::GetMicrosecondsPerTick();
			if(_time >= 500000)
			{
				if(spinlock_try_lock(&_internalLock))
				{
					for(int i = 0; i < Thread::__PriorityClassMax; i ++)
					{
						std::intrusive_list<Thread>::member *entry = _threads[i].head();
						while(entry)
						{
							SchedulingData *temp = entry->get()->GetSchedulingData<SchedulingData>();
							Task *task = entry->get()->GetTask();

							temp->usage = (temp->usage + task->GetNice()) / 3;

							entry = entry->next();
						}
					}

					_time = 0;
					spinlock_unlock(&_internalLock);
				}
			}

			// Update the scheduling decision
			MakeSchedulingDecision();

			_firstRun = false;
			_needsReschedule = false;

			thread = _nextThread;
			_activeThread = thread;

			data = thread->GetSchedulingData<SchedulingData>();
			data->needsWakeup = false;

			Task *task = thread->GetTask();
			Sys::Trampoline *trampoline = _cpu->GetTrampoline();

			trampoline->pageDirectory = task->GetDirectory()->GetPhysicalDirectory();
			trampoline->tss.esp0 = thread->GetESP() + sizeof(Sys::CPUState);
			
			return thread->GetESP();
		}

		Thread *GetActiveThread() const
		{
			return _activeThread;
		}



		void PushCommand(SchedulerCommand &&command)
		{
			if(_cpu == Sys::CPU::GetCurrentCPU())
			{
				RunCommand(command);
				return;
			}

			spinlock_lock(&_commandLock);
			_commands.push_back(std::move(command));
			spinlock_unlock(&_commandLock);

			Notify();
		}

		bool __WorkCommandQueue()
		{
			spinlock_lock(&_commandLock);
			size_t count = _commands.size();

			for(size_t i = 0; i < count; i ++)
			{
				SchedulerCommand &command = _commands.at(i);
				RunCommand(command);
			}

			_commands.clear();

			bool needsReschedule = _needsReschedule;
			_needsReschedule = false;

			spinlock_unlock(&_commandLock);

			return needsReschedule;
		}

		void Disable()
		{
			_enabled.store(false, std::memory_order_release);
		}
		void Enable()
		{
			_enabled.store(true, std::memory_order_release);
		}

	private:
		inline bool CanScheduleThread(Task *task, SchedulingData *data)
		{
			return (data->blocks == 0 && !data->forcedDown && task->GetState() == Task::State::Running);
		}

		void MakeSchedulingDecision()
		{
			if(!spinlock_try_lock(&_internalLock))
				return;

			Thread *thread = _activeThread;
			Task *task = thread->GetTask();

			bool needsReschedule;

			if(task->GetState() == Task::State::Died)
			{
				RemoveThread(thread);
				task->MarkThreadExit(thread);

				thread = nullptr;
				task = nullptr;
				needsReschedule = true;
			}
			else
			{
				SchedulingData *data = thread->GetSchedulingData<SchedulingData>();

				if(data->usage >= 50) // Todo: This should probably be priority dependent
				{
					data->forcedDown = true;
					_hasForcedDown = true;
				}

				needsReschedule = !CanScheduleThread(thread->GetTask(), data);
				if(!needsReschedule)
				{
					// Recalculate the threads priority every 5 ticks
					if((data->usage % 5) == 0)
						data->priority = (data->usage / 4) + task->GetNice();
				}

				// If we are in the idle priority class, unblock all force blocked threads
				if(data->priorityClass == Thread::PriorityClassIdle && _hasForcedDown)
				{
					for(int i = 0; i < Thread::__PriorityClassMax; i ++)
					{
						std::intrusive_list<Thread>::member *entry = _threads[i].head();
						while(entry)
						{
							SchedulingData *temp = entry->get()->GetSchedulingData<SchedulingData>();
							temp->forcedDown = false;

							entry = entry->next();
						}
					}

					_hasForcedDown = false;
				}
			}

			// See if there is a thread with a higher priority that we can schedule
			Thread *newThread = nullptr;

			for(int i = 0; i < Thread::__PriorityClassMax; i ++)
			{
				Thread *lowest = nullptr;
				SchedulingData *lowestData = nullptr;

				std::intrusive_list<Thread>::member *entry = _threads[i].head();
				while(entry)
				{
					Thread *temp = entry->get();
					SchedulingData *tempData = temp->GetSchedulingData<SchedulingData>();

					if(CanScheduleThread(temp->GetTask(), tempData))
					{
						if(tempData->needsWakeup)
						{
							lowestData = tempData;
							lowest = temp;

							break;
						}

						if(!lowestData || tempData->priority < lowestData->priority)
						{
							lowestData = tempData;
							lowest = temp;
						}
					}
					else if(temp->GetTask()->GetState() == Task::State::Died)
					{
						entry = entry->next();

						RemoveThread(temp);
						temp->GetTask()->MarkThreadExit(temp);

						continue;
					}

					entry = entry->next();
				}

				if(lowest)
				{
					newThread = lowest;
					break;
				}
			}

			if(needsReschedule && (newThread == nullptr || newThread == thread))
				newThread = _idleThread;
			
			_nextThread = newThread;
			
			spinlock_unlock(&_internalLock);
		}

		void InsertThread(Thread *thread)
		{
			SchedulingData *data = new SchedulingData(thread);
			data->usage = 0;
			data->priority = 0;
			data->priorityClass = thread->GetPriorityClass();
			data->pinnedCPU = nullptr;
			data->runningCPU = _cpu;
			data->blocks = 0;
			data->forcedDown = false;
			data->needsWakeup = false;

			thread->SetSchedulingData(data);
			_threads[data->priorityClass].push_back(data->schedulerEntry);
		}
		void RemoveThread(Thread *thread)
		{
			SchedulingData *data = thread->GetSchedulingData<SchedulingData>();

			_threads[data->priorityClass].erase(data->schedulerEntry);
			thread->SetSchedulingData(nullptr);

			if(_activeThread == thread)
				_needsReschedule = true;

			delete data;
		}

		void BlockThread(Thread *thread)
		{
			SchedulingData *data = thread->GetSchedulingData<SchedulingData>();
			data->blocks ++;

			if(_activeThread == thread)
				_needsReschedule = true;
		}
		void UnblockThread(Thread *thread)
		{
			SchedulingData *data = thread->GetSchedulingData<SchedulingData>();
			if((-- data->blocks) == 0)
			{
				data->forcedDown = false;
				data->needsWakeup = true;

				_needsReschedule = true;
			}
		}


		void RunCommand(const SchedulerCommand &command)
		{
			switch(command.command)
			{
				case SchedulerCommand::Command::InsertThread:
					InsertThread(command.thread);
					break;
				case SchedulerCommand::Command::RemoveThread:
					RemoveThread(command.thread);
					break;
				case SchedulerCommand::Command::BlockThread:
					BlockThread(command.thread);
					break;
				case SchedulerCommand::Command::UnblockThread:
					UnblockThread(command.thread);
					break;
			}
		}

		void Notify()
		{
			Sys::APIC::SendIPI(0x41, _cpu);
		}

		Sys::CPU *_cpu;
		std::intrusive_list<Thread> _threads[Thread::__PriorityClassMax];
		uint32_t _time;
		Thread *_activeThread;
		Thread *_idleThread;
		Thread *_nextThread;
		bool _firstRun;
		bool _hasForcedDown;
		bool _needsReschedule;
		std::atomic<bool> _enabled;

		spinlock_t _internalLock;
		spinlock_t _commandLock;
		std::vector<SchedulerCommand> _commands;
	};

	// ---------------
	// SMP Scheduler
	// Responsible for coordinating the CPU schedulers
	// ---------------

	static SMPScheduler *_sharedScheduler = nullptr;

	SMPScheduler::SMPScheduler() :
		_schedulerCount(Sys::CPU::GetCPUCount())
	{
		spinlock_init(&_moveLock);

		void *data = kalloc(_schedulerCount * sizeof(CPUScheduler));

		CPUScheduler *proxies = reinterpret_cast<CPUScheduler *>(data);

		for(size_t i = 0; i < CONFIG_MAX_CPUS; i ++)
		{
			Sys::CPU *cpu = Sys::CPU::GetCPUWithID(i);

			if(!cpu)
				continue;

			if(cpu->GetFlags() & Sys::CPU::Flags::Running)
				_schedulerMap[i] = new(proxies + i) CPUScheduler(cpu);
		}

		Sys::SetInterruptHandler(0x23, &SMPScheduler::DoReschedule);
		Sys::SetInterruptHandler(0x41, &SMPScheduler::DoWorkqueue);

		_sharedScheduler = this;
	}

	void SMPScheduler::ActivateCPU(Sys::CPU *cpu)
	{
		CPUScheduler *scheduler = _sharedScheduler->_schedulerMap[cpu->GetID()];
		Task *task = GetKernelTask();

		KernReturn<Thread *> thread;

		if((thread = task->AttachThread((Thread::Entry)&IdleTask, Thread::PriorityClass::PriorityClassIdle, 0, nullptr)).IsValid() == false)
			panic("Failed to activate CPU %d", cpu->GetID());

		scheduler->_idleThread = thread;
		scheduler->_activeThread = thread;
		scheduler->_nextThread = thread;
		scheduler->_firstRun = true;
	}


	uint32_t SMPScheduler::DoWorkqueue(uint32_t esp, Sys::CPU *cpu)
	{
		CPUScheduler *scheduler = _sharedScheduler->_schedulerMap[cpu->GetID()];
		
		if(scheduler->__WorkCommandQueue())
			esp = scheduler->Schedule(esp);

		return esp;
	}
	uint32_t SMPScheduler::DoReschedule(uint32_t esp, Sys::CPU *cpu)
	{
		return _sharedScheduler->ScheduleOnCPU(esp, cpu);
	}


	Task *SMPScheduler::GetActiveTask() const
	{
		return GetActiveThread()->GetTask();
	}
	Thread *SMPScheduler::GetActiveThread() const
	{
		Sys::CPU *cpu = Sys::CPU::GetCurrentCPU();
		return _schedulerMap[cpu->GetID()]->GetActiveThread();
	}


	uint32_t SMPScheduler::ScheduleOnCPU(uint32_t esp, Sys::CPU *cpu)
	{
		CPUScheduler *scheduler = _schedulerMap[cpu->GetID()];
		return scheduler->Schedule(esp);
	}
	uint32_t SMPScheduler::PokeCPU(uint32_t esp, Sys::CPU *cpu)
	{
		CPUScheduler *scheduler = _schedulerMap[cpu->GetID()];

		if(scheduler->_needsReschedule)
			return scheduler->Schedule(esp);

		return esp;
	}

	void SMPScheduler::RescheduleCPU(Sys::CPU *cpu)
	{
		Sys::APIC::SendIPI(0x23, cpu);
	}

	void SMPScheduler::DisableCPU(Sys::CPU *cpu)
	{
		CPUScheduler *scheduler = _schedulerMap[cpu->GetID()];
		scheduler->Disable();
	}
	void SMPScheduler::EnableCPU(Sys::CPU *cpu)
	{
		CPUScheduler *scheduler = _schedulerMap[cpu->GetID()];
		scheduler->Enable();
	}


	void SMPScheduler::BlockThread(Thread *thread)
	{
		SchedulingData *data = thread->GetSchedulingData<SchedulingData>();
		if(!data)
			return;
		
		Sys::CPU *cpu = data->runningCPU;

		SchedulerCommand command(SchedulerCommand::Command::BlockThread, thread);
		_schedulerMap[cpu->GetID()]->PushCommand(std::move(command));
	}
	void SMPScheduler::UnblockThread(Thread *thread)
	{
		SchedulingData *data = thread->GetSchedulingData<SchedulingData>();
		if(!data)
			return;

		Sys::CPU *cpu = data->runningCPU;

		SchedulerCommand command(SchedulerCommand::Command::UnblockThread, thread);
		_schedulerMap[cpu->GetID()]->PushCommand(std::move(command));
	}


	void SMPScheduler::AddThread(Thread *thread)
	{
		Sys::CPU *cpu = Sys::CPU::GetCurrentCPU();
		CPUScheduler *scheduler = _schedulerMap[cpu->GetID()];

		scheduler->PushCommand(SchedulerCommand(SchedulerCommand::Command::InsertThread, thread));
	}
	void SMPScheduler::RemoveThread(Thread *thread)
	{
		SchedulingData *data = thread->GetSchedulingData<SchedulingData>();
		Sys::CPU *cpu = data->runningCPU;

		SchedulerCommand command(SchedulerCommand::Command::RemoveThread, thread);
		_schedulerMap[cpu->GetID()]->PushCommand(std::move(command));
	}
}
