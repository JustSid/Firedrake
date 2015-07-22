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
#include <objects/IONumber.h>
#include "task.h"
#include "thread.h"

namespace OS
{
	constexpr vm_address_t kThreadStackLimit = 0xffff000;

	IODefineMeta(Thread, IO::Object)

	void __ThreadIPCCallback(IPC::Port *port, IPC::Message *message)
	{
	}

	KernReturn<Thread *> Thread::Init(Task *task, Entry entry, PriorityClass priority, size_t stackPages, IO::Array *parameters)
	{
		if(!IO::Object::Init())
			return Error(KERN_FAILURE);

		_task  = task;
		_entry = entry;
		_esp   = 0;
		_priority = priority;
		_kernelStack = nullptr;
		_kernelStackVirtual = nullptr;
		_userStack = nullptr;
		_userStackVirtual = nullptr;

		_tid = _task->_tidCounter.fetch_add(1);

		IPC::Space *space = _task->GetIPCSpace();
		space->Lock();
		_threadPort = space->AllocateCallbackPort(&__ThreadIPCCallback);
		_threadSendPort = space->AllocateSendPort(_threadPort, IPC::Port::Right::Send, IPC_PORT_NULL);
		space->Unlock();

		if(_task->_ring3)
		{
			_userStackPages   = std::min<size_t>(64, std::max<size_t>(24, stackPages));
			_kernelStackPages = 1;

			KernReturn<void> result = InitializeForRing3(parameters);
			if(!result.IsValid())
				return result.GetError();
		}
		else
		{
			_kernelStackPages = std::min<size_t>(32, std::max<size_t>(12, stackPages));
			_userStackPages   = 0;

			KernReturn<void> result = InitializeForRing0(parameters);
			if(!result.IsValid())
				return result.GetError();
		}

		return this;
	}

	void Thread::Dealloc()
	{
		IO::Object::Dealloc();
	}

	KernReturn<void> Thread::InitializeForRing3(IO::Array *parameters)
	{
		{
			KernReturn<uintptr_t> paddress;
			KernReturn<vm_address_t> vaddress;

			paddress = Sys::PM::Alloc(_userStackPages);
			if(paddress.IsValid() == false)
			{
				kprintf("Failed to allocate %i physicial user stack pages\n", _userStackPages);
				return paddress.GetError();
			}

			vaddress = _task->_directory->AllocLimit(paddress, kThreadStackLimit, Sys::VM::kUpperLimit, _userStackPages, kVMFlagsUserlandRW);
			if(vaddress.IsValid() == false)
			{
				Sys::PM::Free(paddress, _userStackPages);

				kprintf("Failed to allocate %i virtual user stack pages\n", _userStackPages);
				return vaddress.GetError();
			}

			_userStack = reinterpret_cast<uint8_t *>(paddress.Get());
			_userStackVirtual = reinterpret_cast<uint8_t *>(vaddress.Get());

			// Kernel stack
			paddress = Sys::PM::Alloc(_kernelStackPages);
			vaddress = _task->_directory->AllocTwoSidedLimit(paddress, kThreadStackLimit, Sys::VM::kUpperLimit, _kernelStackPages, kVMFlagsKernel);
			
			_kernelStack = reinterpret_cast<uint8_t *>(paddress.Get());
			_kernelStackVirtual = reinterpret_cast<uint8_t *>(vaddress.Get());
		}

		// Initialize the process stack
		Sys::VM::Directory *kernelDir = Sys::VM::Directory::GetKernelDirectory();
		KernReturn<vm_address_t> vaddress = kernelDir->Alloc(reinterpret_cast<uintptr_t>(_userStack), _userStackPages, kVMFlagsKernel);

		if(!vaddress.IsValid())
			return vaddress.GetError();
		
		size_t parameterSize = 0;
		uint8_t *ptr = reinterpret_cast<uint8_t *>(vaddress.Get());
		memset(ptr, 0, _userStackPages);

		if(parameters)
		{
			uint8_t *stackEnd = ParseParameters(parameters, ptr + (_userStackPages * VM_PAGE_SIZE));
			parameterSize = (ptr + (_userStackPages * VM_PAGE_SIZE)) - stackEnd;
		}

		kernelDir->Free(vaddress, _userStackPages);

		// Prepare the interrupt stack
		size_t size     = _kernelStackPages * VM_PAGE_SIZE;
		uint32_t *stack = reinterpret_cast<uint32_t *>(_kernelStackVirtual + size);

		*(-- stack) = 0x23; // ss
		*(-- stack) = (uint32_t)(_userStackVirtual + ((_userStackPages * VM_PAGE_SIZE) - (4 + parameterSize)));   // esp
		*(-- stack) = 0x200; // eflags
		*(-- stack) = 0x1b; // cs
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

		*(-- stack) = 0x23;
		*(-- stack) = 0x23;
		*(-- stack) = 0x23;
		*(-- stack) = 0x23;

		_esp = reinterpret_cast<uint32_t>(_kernelStackVirtual + size) - sizeof(Sys::CPUState);
		return ErrorNone;
	}

	KernReturn<void> Thread::InitializeForRing0(IO::Array *parameters)
	{
		{
			KernReturn<uintptr_t> paddress;
			KernReturn<vm_address_t> vaddress;

			paddress = Sys::PM::Alloc(_kernelStackPages);
			if(paddress.IsValid() == false)
			{
				kprintf("Failed to allocate %i physicial kernel stack pages\n", _kernelStackPages);
				return paddress.GetError();
			}

			vaddress = _task->_directory->AllocLimit(paddress, kThreadStackLimit, Sys::VM::kUpperLimit, _kernelStackPages, kVMFlagsKernel);
			if(vaddress.IsValid() == false)
			{
				Sys::PM::Free(paddress, _kernelStackPages);

				kprintf("Failed to allocate %i virtual kernel stack pages\n", _kernelStackPages);
				return vaddress.GetError();
			}

			_kernelStack = reinterpret_cast<uint8_t *>(paddress.Get());
			_kernelStackVirtual = reinterpret_cast<uint8_t *>(vaddress.Get());
		}

		size_t size = _kernelStackPages * VM_PAGE_SIZE;
		uint32_t *stack = reinterpret_cast<uint32_t *>(_kernelStackVirtual + size);

		if(parameters)
			stack = reinterpret_cast<uint32_t *>(ParseParameters(parameters, reinterpret_cast<uint8_t *>(stack)));

		*(-- stack) = 0x10; // ss
		*(-- stack) = 0x0;   // esp
		*(-- stack) = 0x200; // eflags
		*(-- stack) = 0x8; // cs
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

		*(-- stack) = 0x10;
		*(-- stack) = 0x10;
		*(-- stack) = 0x0;
		*(-- stack) = 0x0;

		_esp = reinterpret_cast<uint32_t>(stack);
		return ErrorNone;
	}

	uint8_t *Thread::ParseParameters(IO::Array *parameters, uint8_t *stack)
	{
		size_t count = parameters->GetCount();
		size_t size = 0;

		// Calculate the size requirements
		for(size_t i = 0; i < count; i ++)
		{
			IO::Number *parameter = parameters->GetObjectAtIndex<IO::Number>(i); // For now, only IO::Number is supported

			switch(parameter->GetType())
			{
				// All promoted to 32bit types
				case IO::Number::Type::Int8:
				case IO::Number::Type::Int16:
				case IO::Number::Type::Int32:
				case IO::Number::Type::Uint8:
				case IO::Number::Type::Uint16:
				case IO::Number::Type::Uint32:
				case IO::Number::Type::Boolean:
					size += sizeof(uint32_t);
					break;

				case IO::Number::Type::Int64:
				case IO::Number::Type::Uint64:
					size += sizeof(uint64_t);
					break;
			}
		}

		// Push onto the stack
		stack -= size;

		for(size_t i = 0; i < count; i ++)
		{
			IO::Number *parameter = parameters->GetObjectAtIndex<IO::Number>(i); // For now, only IO::Number is supported

			switch(parameter->GetType())
			{
				// All promoted to 32bit types
				case IO::Number::Type::Int8:
				case IO::Number::Type::Int16:
				case IO::Number::Type::Int32:
				case IO::Number::Type::Uint8:
				case IO::Number::Type::Uint16:
				case IO::Number::Type::Uint32:
				case IO::Number::Type::Boolean:
				{
					uint32_t *temp = reinterpret_cast<uint32_t *>(stack);
					*(temp ++) = parameter->GetUint32Value();
					stack = reinterpret_cast<uint8_t *>(temp);

					break;
				}

				case IO::Number::Type::Int64:
				case IO::Number::Type::Uint64:
				{
					uint64_t *temp = reinterpret_cast<uint64_t *>(stack);
					*(temp ++) = parameter->GetUint64Value();
					stack = reinterpret_cast<uint8_t *>(temp);

					break;
				}
			}
		}

		return (stack - size);
	}

	void Thread::SetESP(uint32_t esp)
	{
		_esp = esp;
	}

	void Thread::SetSchedulingData(void *data)
	{
		_schedulingData = data;
	}
}
