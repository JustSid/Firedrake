//
//  IPCSyscall.cpp
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
#include <os/scheduler/scheduler.h>
#include <kern/kprintf.h>
#include "IPCSyscall.h"

namespace OS
{
	namespace IPC
	{
		KernReturn<uint32_t> Syscall_IPCTaskPort(Thread *thread, IPCPortCallArgs *arguments)
		{
			OS::SyscallScopedMapping portMapping(thread->GetTask(), arguments->port, sizeof(ipc_port_t));

			ipc_port_t *port = portMapping.GetMemory<ipc_port_t>();
			*port = thread->GetTask()->GetTaskPort()->GetPortName();

			return KERN_SUCCESS;
		}

		KernReturn<uint32_t> Syscall_IPCThreadPort(Thread *thread, IPCPortCallArgs *arguments)
		{
			OS::SyscallScopedMapping portMapping(thread->GetTask(), arguments->port, sizeof(ipc_port_t));

			ipc_port_t *port = portMapping.GetMemory<ipc_port_t>();
			*port = thread->GetThreadPort()->GetPortName();

			return KERN_SUCCESS;
		}

		KernReturn<uint32_t> Syscall_IPCMessage(Thread *thread, IPCReadWriteArgs *arguments)
		{
			if(arguments->size == 0 || (arguments->mode != IPC_WRITE && arguments->mode != IPC_READ))
				return KERN_INVALID_ARGUMENT;

			OS::SyscallScopedMapping headerMapping(thread->GetTask(), arguments->header, arguments->size + sizeof(ipc_header_t));
			ipc_header_t *header = headerMapping.GetMemory<ipc_header_t>();
			
			KernReturn<void> result;

			switch(arguments->mode)
			{
				case IPC_READ:
				{
					Message *message = Message::Alloc()->Init(header);
					result = Read(thread->GetTask(), message);
					message->Release();
					
					break;
				}

				case IPC_WRITE:
				{
					Message *message = Message::Alloc()->Init(header);
					result = Write(thread->GetTask(), message);
					message->Release();

					break;
				}
			}

			if(!result.IsValid())
				return result.GetError();

			return KERN_SUCCESS;
		}

		KernReturn<uint32_t> Syscall_IPCAllocatePort(Thread *thread, IPCAllocatePortArgs *arguments)
		{
			OS::SyscallScopedMapping portMapping(thread->GetTask(), arguments->result, sizeof(ipc_port_t));

			ipc_port_t *port = portMapping.GetMemory<ipc_port_t>();
			Task *task = thread->GetTask();
			System *system = task->GetTaskSystem();

			system->Lock();
			Port *result = system->AddPort(arguments->name, static_cast<Port::Rights>(arguments->options));
			system->Unlock();

			if(result)
			{
				*port = result->GetPortName();
				return KERN_SUCCESS;
			}

			*port = IPC_PORT_NULL;
			return KERN_RESOURCE_EXISTS;
		}
	}
}
