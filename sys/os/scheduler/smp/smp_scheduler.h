//
//  smp_scheduler.h
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

#ifndef _SMP_SCHEDULER_H_
#define _SMP_SCHEDULER_H_

#include <prefix.h>
#include <os/scheduler/scheduler.h>
#include <libcpp/intrusive_list.h>
#include <libc/sys/spinlock.h>

namespace OS
{
	class SMPScheduler : public Scheduler
	{
	public:
		SMPScheduler();

		Task *GetActiveTask() const final;
		Thread *GetActiveThread() const final;

		uint32_t ScheduleOnCPU(uint32_t esp, Sys::CPU *cpu) final;
		uint32_t PokeCPU(uint32_t esp, Sys::CPU *cpu) final;
		
		void RescheduleCPU(Sys::CPU *cpu) final;
		void ActivateCPU(Sys::CPU *cpu) final;

		void BlockThread(Thread *thread) final;
		void UnblockThread(Thread *thread) final;

		void AddThread(Thread *thread) final;
		void RemoveThread(Thread *thread) final;

		void DisableCPU(Sys::CPU *cpu) final;
		void EnableCPU(Sys::CPU *cpu) final;

	private:
		static uint32_t DoWorkqueue(uint32_t esp, Sys::CPU *cpu);
		static uint32_t DoReschedule(uint32_t esp, Sys::CPU *cpu);

		struct SchedulingData
		{
			SchedulingData(Thread *thread) :
				schedulerEntry(thread)
			{}

			uint32_t usage;
			uint32_t priority;
			Thread::PriorityClass priorityClass;
			std::intrusive_list<Thread>::member schedulerEntry;
			Sys::CPU *pinnedCPU;
			Sys::CPU *runningCPU;
			uint32_t blocks;
			bool forcedDown;
			bool needsWakeup;
		};

		struct SchedulerCommand
		{
			enum class Command
			{
				InsertThread,
				RemoveThread,
				BlockThread,
				UnblockThread
			};

			SchedulerCommand(Command tcommand, Thread *tthread) :
				command(tcommand),
				thread(tthread->Retain())
			{}
			~SchedulerCommand()
			{
				IO::SafeRelease(thread);
			}

			SchedulerCommand &operator =(const SchedulerCommand &other)
			{
				thread = IO::SafeRetain(other.thread);
				command = other.command;
				return *this;
			}	

			SchedulerCommand(SchedulerCommand &&other) :
				command(other.command),
				thread(other.thread)
			{
				other.thread = nullptr;
			}

			SchedulerCommand &operator =(SchedulerCommand &&other)
			{
				thread = other.thread;
				command = other.command;

				other.thread = nullptr;
				return *this;
			}			

			Command command;
			Thread *thread;
		};

		class CPUScheduler;

		size_t _schedulerCount;
		CPUScheduler *_schedulerMap[CONFIG_MAX_CPUS]; // The CPU number corresponds with the array index
		spinlock_t _moveLock;
	};
}

#endif /* _SMP_SCHEDULER_H_ */
