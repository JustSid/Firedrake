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
#include <machine/memory/memory.h>
#include <kern/scheduler/scheduler.h>
#include <kern/kprintf.h>
#include <kern/kalloc.h>
#include "syscall.h"

namespace Core
{
	struct SyscallEntry
	{
		SyscallHandler handler;
		size_t argSize;
		bool unixSyscall; // Expects errno
	};

	SyscallEntry _entries[__SYS_MaxSyscalls];
	spinlock_t _entryLock;


	kern_return_t SetSyscallHandler(uint32_t number, SyscallHandler handler, size_t argSize, bool isUnixStyle)
	{
		if(number >= __SYS_MaxSyscalls)
			return KERN_INVALID_ARGUMENT;

		spinlock_lock(&_entryLock);

		_entries[number].handler     = handler;
		_entries[number].argSize     = argSize;
		_entries[number].unixSyscall = isUnixStyle;

		spinlock_unlock(&_entryLock);

		return KERN_SUCCESS;
	}

	uint32_t HandleSyscall(uint32_t esp, Sys::CPU *cpu)
	{
		spinlock_lock(&_entryLock);

		Sys::CPUState *state = cpu->GetLastState();
		SyscallEntry entry = _entries[state->eax];

		spinlock_unlock(&_entryLock);

		if(!entry.handler)
		{
			kprintf("No one known handler for syscall %i\n", state->eax);
			return esp; // TODO: Should probably kill the process
		}



		Thread *thread = Scheduler::GetActiveThread();
		thread->SetESP(esp);

		void *arguments = nullptr;
		
		// Copy the arguments. Ideally we would transfer most arguments in registers and not the stack to avoid
		// this mapping madness, but oh well, who cares?!	
		if(entry.argSize > 0)
		{
			vm_address_t address;

			uint32_t offset = ((uint8_t *)state->esp) - thread->GetUserStackVirtual();
			uintptr_t stack = ((uintptr_t)thread->GetUserStack()) + offset + 8; // Jump over the return address and syscall number

			uintptr_t aligned = VM_PAGE_ALIGN_DOWN(stack);
			size_t pages = VM_PAGE_COUNT((stack - aligned) + entry.argSize);

			Sys::VM::Directory *directory = Sys::VM::Directory::GetKernelDirectory();
			kern_return_t result = directory->Alloc(address, aligned, pages, kVMFlagsKernel);

			if(result != KERN_SUCCESS)
			{
				state->eax = -1;
				state->ecx = (entry.unixSyscall) ? ENOMEM : KERN_NO_MEMORY;

				return esp;
			}

			arguments = kalloc(entry.argSize);
			if(!arguments)
			{
				directory->Free(address, pages);

				state->eax = -1;
				state->ecx = (entry.unixSyscall) ? ENOMEM : KERN_NO_MEMORY;

				return esp;
			}

			memcpy(arguments, (uint8_t *)(address + (stack - aligned)), entry.argSize);
			directory->Free(address, pages);
		}

		// Call the actual handler of the system call
		kern_return_t result = entry.handler(esp, state->eax, arguments);
		if(result != KERN_SUCCESS)
		{
			state->ecx = (entry.unixSyscall) ? ErrnoFromReturn(result) : result;
		}

		if(arguments)
			kfree(arguments);

		return esp;
	}

	kern_return_t SyscallInit()
	{
		memset(_entries, 0, __SYS_MaxSyscalls * sizeof(SyscallEntry));

		Sys::SetInterruptHandler(0x80, HandleSyscall);

		return KERN_SUCCESS;
	}
}
