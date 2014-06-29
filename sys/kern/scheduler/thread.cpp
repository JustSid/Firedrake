//
//  thread.cpp
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

#include <libc/string.h>
#include <libcpp/algorithm.h>
#include <libcpp/new.h>
#include <machine/memory/memory.h>
#include <machine/cpu.h>
#include <kern/kprintf.h>
#include "task.h"
#include "thread.h"

namespace Core
{
	constexpr vm_address_t kThreadStackLimit = 0xffff000;

	Thread::Thread(Task *task, Entry entry, size_t stackPages) :
		_task(task),
		_lock(SPINLOCK_INIT),
		_quantum(0),
		_entry(entry),
		_pinnedCPU(nullptr),
		_runningCPU(nullptr),
		_schedulerEntry(this),
		_taskEntry(this),
		_kernelStack(nullptr),
		_kernelStackVirtual(nullptr),
		_userStack(nullptr),
		_userStackVirtual(nullptr)
	{
		_tid = _task->_tidCounter.fetch_add(1);
		_esp = 0;

		if(_task->_ring3)
		{
			_userStackPages   = std::min<size_t>(64, std::max<size_t>(24, stackPages));
			_kernelStackPages = 4;
		}
		else
		{
			_kernelStackPages = std::min<size_t>(32, std::max<size_t>(12, stackPages));
			_userStackPages   = 0;
		}
	}

	Thread::~Thread()
	{}

	kern_return_t Thread::Create(Thread *&thread, Task *task, Entry entry, size_t stackPages)
	{
		void *buffer = kalloc(sizeof(Thread));
		if(!buffer)
			return KERN_NO_MEMORY;

		thread = new(buffer) Thread(task, entry, stackPages);

		kern_return_t result;
		if((result = thread->Initialize()) != KERN_SUCCESS)
		{
			delete thread;
			return result;
		}
		
		return KERN_SUCCESS;
	}

	kern_return_t Thread::Initialize()
	{
		if(!_task->_ring3)
		{
			uintptr_t paddress;
			vm_address_t vaddress;
			kern_return_t result;

			result = Sys::PM::Alloc(paddress, _kernelStackPages);
			if(result != KERN_SUCCESS)
			{
				kprintf("Failed to allocate %i physicial kernel stack pages\n", _kernelStackPages);
				return result;
			}

			result = _task->_directory->AllocLimit(vaddress, paddress, kThreadStackLimit, Sys::VM::kUpperLimit, _kernelStackPages, kVMFlagsKernel);
			if(result != KERN_SUCCESS)
			{
				Sys::PM::Free(paddress, _kernelStackPages);

				kprintf("Failed to allocate %i virtual kernel stack pages\n", _kernelStackPages);
				return result;
			}

			_kernelStack = reinterpret_cast<uint8_t *>(paddress);
			_kernelStackVirtual = reinterpret_cast<uint8_t *>(vaddress);
		}

		InitializeKernelStack(_task->_ring3);
		return KERN_SUCCESS;
	}

	void Thread::InitializeKernelStack(bool ring3)
	{
		size_t size     = _kernelStackPages * VM_PAGE_SIZE;
		uint32_t *stack = reinterpret_cast<uint32_t *>(_kernelStackVirtual + size);

		*(-- stack) = ring3 ? 0x23 : 0x10; // ss
		*(-- stack) = 0x0;   // esp
		*(-- stack) = 0x200; // eflags
		*(-- stack) = ring3 ? 0x1b : 0x8; // cs
		*(-- stack) = _entry; // eip

		*(-- stack) = 0x0;
		*(-- stack) = 0x0;

		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;

		*(-- stack) = ring3 ? 0x23 : 0x10;
		*(-- stack) = ring3 ? 0x23 : 0x10;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;

		_esp = ring3 ? reinterpret_cast<uint32_t>(_kernelStackVirtual + size) - sizeof(Sys::CPUState) : reinterpret_cast<uint32_t>(stack);
	}

	void Thread::SetQuantum(int8_t quantum)
	{
		_quantum = quantum;
	}
	void Thread::SetESP(uint32_t esp)
	{
		_esp = esp;
	}
	void Thread::SetRunningCPU(Sys::CPU *cpu)
	{
		_runningCPU = cpu;
	}
	void Thread::SetPinnedCPU(Sys::CPU *cpu)
	{
		_pinnedCPU = cpu;
	}

	bool Thread::IsSchedulable(Sys::CPU *cpu) const
	{
		if(_runningCPU || (_pinnedCPU && _pinnedCPU != cpu))
			return false;

		return true;
	}

	void Thread::Lock()
	{
		spinlock_lock(&_lock);
	}
	void Thread::Unlock()
	{
		spinlock_unlock(&_lock);
	}
}
