//
//  thread.h
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

#ifndef _THREAD_H_
#define _THREAD_H_

#include <prefix.h>
#include <libc/stddef.h>
#include <libc/stdint.h>
#include <libc/sys/types.h>
#include <libcpp/atomic.h>
#include <libcpp/queue.h>
#include <machine/cpu.h>
#include <kern/kern_return.h>
#include <libc/sys/spinlock.h>

namespace OS
{
	class Task;

	class Thread
	{
	public:
		friend class Task;
		typedef uint32_t Entry;

		~Thread();

		void Lock();
		void Unlock();

		void SetQuantum(int8_t quantum);
		void SetESP(uint32_t esp);
		void SetRunningCPU(Sys::CPU *cpu);
		void SetPinnedCPU(Sys::CPU *cpu);

		bool IsSchedulable(Sys::CPU *cpu) const;

		Task *GetTask() const { return _task; }
		tid_t GetTid() const { return _tid; }
		uint32_t GetESP() const { return _esp; }

		int8_t ReduceQuantum() { return (_quantum --); }
		int8_t GetQuantum() const { return _quantum; }

		uint8_t *GetUserStack() const { return _userStack; }
		uint8_t *GetUserStackVirtual() const { return _userStackVirtual; }
		uint8_t *GetKernelStack() const { return _kernelStack; }
		uint8_t *GetKernelStackVirtual() const { return _kernelStackVirtual; }

		size_t GetUserStackPages() const { return _userStackPages; }
		size_t GetKernelStackPages() const { return _kernelStackPages; }

		cpp::queue<Thread>::entry &GetSchedulerEntry() { return _schedulerEntry; }

	private:
		static KernReturn<Thread *> Create(Task *task, Entry entry, size_t stackPages);
		
		Thread(Task *task, Entry entry, size_t stackPages);

		KernReturn<void> Initialize();
		KernReturn<void> InitializeForRing3();
		KernReturn<void> InitializeForRing0();

		Task *_task;
		tid_t _tid;
		spinlock_t _lock;

		Sys::CPU *_pinnedCPU;
		Sys::CPU *_runningCPU;

		cpp::queue<Thread>::entry _schedulerEntry;
		cpp::queue<Thread>::entry _taskEntry;

		int8_t _quantum;

		size_t _userStackPages;
		uint8_t *_userStack;
		uint8_t *_userStackVirtual;

		size_t _kernelStackPages;
		uint8_t *_kernelStack;
		uint8_t *_kernelStackVirtual;

		uint32_t _esp;
		uint32_t _entry;
	};
}

#endif /* _THREAD_H_ */
