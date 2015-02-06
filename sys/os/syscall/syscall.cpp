//
//  syscall.cpp
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

#include <libc/sys/spinlock.h>
#include <libc/string.h>
#include <machine/interrupts/interrupts.h>
#include <os/scheduler/scheduler.h>
#include <kern/kprintf.h>
#include <kern/kalloc.h>
#include "syscall.h"

namespace OS
{
	extern SyscallTrap _syscallTrapTable[];
	extern SyscallTrap _kernTrapTable[];

	SyscallScopedMapping::SyscallScopedMapping(const void *pointer, size_t size) :
		_address(0x0),
		_pointer(0x0),
		_pages(0)
	{
		Task *task = Scheduler::GetActiveTask();

		vm_address_t ptr = reinterpret_cast<vm_address_t>(const_cast<void *>(pointer));

		vm_address_t base = VM_PAGE_ALIGN_DOWN(ptr);
		vm_address_t offset = ptr - base;

		if(!base)
			return;

		Sys::VM::Directory *directory = task->GetDirectory();
		KernReturn<uintptr_t> physical = directory->ResolveAddress(base);

		if(!physical.IsValid())
			return;

		_pages = VM_PAGE_COUNT(size + offset);
		_address = Sys::VM::Directory::GetKernelDirectory()->Alloc(physical, _pages, kVMFlagsKernel);
		_pointer = _address + offset;
	}

	SyscallScopedMapping::~SyscallScopedMapping()
	{
		if(_address)
			Sys::VM::Directory::GetKernelDirectory()->Free(_address, _pages);
	}

	uint32_t HandleSyscall(uint32_t esp, Sys::CPU *cpu)
	{
		Sys::CPUState *state = cpu->GetLastState();

		if(state->eax >= 128)
			return esp;

		bool kernelTrap = (state->interrupt == 0x81);
		const SyscallTrap *entry = kernelTrap ? (_kernTrapTable + state->eax) : (_syscallTrapTable + state->eax);

		Thread *thread = Scheduler::GetActiveThread();
		thread->SetESP(esp);

		uint32_t *arguments = nullptr;

		if(entry->argCount)
		{
			arguments = new uint32_t[entry->argCount];
			if(!arguments)
				goto badMemory;

			switch(entry->argCount)
			{
				default:
					arguments[4] = state->ebx;
				case 4:
					arguments[3] = state->edx;
				case 3:
					arguments[2] = state->esi;
				case 2:
					arguments[1] = state->edi;
				case 1:
					arguments[0] = state->ecx;
				case 0:
					break;
			}

			if(entry->argCount > 5)
			{
				// Get all the other arguments from the stack
				size_t count = entry->argCount - 5;

				uint32_t offset = ((uint8_t *)state->esp) - thread->GetUserStackVirtual();
				uintptr_t stack = ((uintptr_t)thread->GetUserStack()) + offset + 44; // Jump to the arguments on the stack

				uintptr_t aligned = VM_PAGE_ALIGN_DOWN(stack);
				size_t pages = VM_PAGE_COUNT((stack - aligned) + count * sizeof(uint32_t));

				Sys::VM::Directory *directory = Sys::VM::Directory::GetKernelDirectory();
				KernReturn<vm_address_t> address = directory->Alloc(aligned, pages, kVMFlagsKernel);

				if(address.IsValid() == false)
				{
					delete[] arguments;
					goto badMemory;
				}

				memcpy(arguments + 5, (uint8_t *)(address + (stack - aligned)), count * sizeof(uint32_t));
				directory->Free(address, pages);
			}
		}

		goto handler;

	badMemory:
		if(kernelTrap)
		{
			state->ecx = ENOMEM;
			state->eax = -1;
		}
		else
		{
			state->eax = KERN_NO_MEMORY;
		}


	handler:

		// Call the actual handler of the system call
		KernReturn<uint32_t> result = entry->handler(esp, arguments);

		if(kernelTrap)
		{
			state->eax = (result.IsValid() == false) ? result.GetError().GetCode() : KERN_SUCCESS;
		}
		else
		{
			if(result.IsValid() == false)
			{
				Error error = result.GetError();
				state->eax = -1;
				state->ecx = error.GetErrno();
			}
			else
			{
				state->eax = result;
				state->ecx = 0;
			}
		}

		delete[] arguments;
		return esp;
	}

	KernReturn<void> SyscallInit()
	{
		Sys::SetInterruptHandler(0x80, HandleSyscall);
		Sys::SetInterruptHandler(0x81, HandleSyscall);

		return ErrorNone;
	}
}
