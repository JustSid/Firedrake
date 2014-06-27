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
#include <machine/memory/memory.h>
#include <machine/cpu.h>
#include <kern/kprintf.h>
#include "task.h"
#include "thread.h"

#define THREAD_STACK_LIMIT 0xffff000

namespace sd
{
	thread_t::thread_t(task_t *task, entry_t entry, size_t stackPages) :
		_task(task),
		_lock(SPINLOCK_INIT),
		_quantum(0),
		_entry(entry),
		_pinned_cpu(nullptr),
		_running_cpu(nullptr),
		_scheduler_entry(this),
		_task_entry(this)
	{
		_tid = _task->_tid_counter.fetch_add(1);
		_esp = 0;

		if(_task->_ring3)
		{
			_user_stack_pages   = std::min<size_t>(64, std::max<size_t>(24, stackPages));
			_kernel_stack_pages = 4;
		}
		else
		{
			_kernel_stack_pages = std::min<size_t>(32, std::max<size_t>(12, stackPages));
			_user_stack_pages   = 0;
		}

		_user_stack   = _user_stack_virtual   = nullptr;
		_kernel_stack = _kernel_stack_virtual = nullptr;
	}

	thread_t::~thread_t()
	{}

	kern_return_t thread_t::initialize()
	{
		if(!_task->_ring3)
		{
			uintptr_t paddress;
			vm_address_t vaddress;
			kern_return_t result;

			result = pm::alloc(paddress, _kernel_stack_pages);
			if(result != KERN_SUCCESS)
			{
				kprintf("Failed to allocate %i physicial kernel stack pages\n", _kernel_stack_pages);
				return result;
			}

			result = _task->_directory->alloc_limit(vaddress, paddress, THREAD_STACK_LIMIT, VM_UPPER_LIMIT, _kernel_stack_pages, VM_FLAGS_KERNEL);
			if(result != KERN_SUCCESS)
			{
				pm::free(paddress, _kernel_stack_pages);

				kprintf("Failed to allocate %i virtual kernel stack pages\n", _kernel_stack_pages);
				return result;
			}

			_kernel_stack = reinterpret_cast<uint8_t *>(paddress);
			_kernel_stack_virtual = reinterpret_cast<uint8_t *>(vaddress);
		}

		initialize_kernel_stack(_task->_ring3);
		return KERN_SUCCESS;
	}

	void thread_t::initialize_kernel_stack(bool ring3)
	{
		size_t size = _kernel_stack_pages * VM_PAGE_SIZE;
		uint32_t *stack = reinterpret_cast<uint32_t *>(_kernel_stack_virtual + size);

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

		_esp = ring3 ? reinterpret_cast<uint32_t>(_kernel_stack_virtual + size) - sizeof(Sys::CPUState) : reinterpret_cast<uint32_t>(stack);
	}

	bool thread_t::is_schedulable(Sys::CPU *cpu) const
	{
		if(_running_cpu || (_pinned_cpu && _pinned_cpu != cpu))
			return false;

		return true;
	}

	void thread_t::lock()
	{
		spinlock_lock(&_lock);
	}

	void thread_t::unlock()
	{
		spinlock_unlock(&_lock);
	}
}
