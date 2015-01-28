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
#include "IPC.h"

namespace OS
{
	namespace IPC
	{
		struct IPCPortCallArgs
		{
			ipc_port_t *port;
		} __attribute__((packed));

		struct IPCReadWriteArgs
		{
			ipc_header_t *header;
			ipc_size_t size;
			int mode;
		} __attribute__((packed));


		KernReturn<uint32_t> Syscall_IPCTaskPort(__unused uint32_t &esp, void *args)
		{
			IPCPortCallArgs *arguments = (IPCPortCallArgs *)args;
			OS::SyscallScopedMapping portMapping(arguments->port, sizeof(ipc_port_t));

			ipc_port_t *port = portMapping.GetMemory<ipc_port_t>();
			*port = Scheduler::GetActiveTask()->GetTaskPort()->GetPortName();

			return KERN_SUCCESS;
		}

		KernReturn<uint32_t> Syscall_IPCThreadPort(__unused uint32_t &esp, void *args)
		{
			IPCPortCallArgs *arguments = (IPCPortCallArgs *)args;
			OS::SyscallScopedMapping portMapping(arguments->port, sizeof(ipc_port_t));

			ipc_port_t *port = portMapping.GetMemory<ipc_port_t>();
			*port = Scheduler::GetActiveThread()->GetThreadPort()->GetPortName();

			return KERN_SUCCESS;
		}

		KernReturn<uint32_t> Syscall_IPCMessage(__unused uint32_t &esp, void *args)
		{
			IPCReadWriteArgs *arguments = (IPCReadWriteArgs *)args;

			if(arguments->size == 0 || (arguments->mode != IPC_WRITE && arguments->mode != IPC_READ))
				return KERN_INVALID_ARGUMENT;

			OS::SyscallScopedMapping headerMapping(arguments->header, arguments->size + sizeof(ipc_header_t));
			ipc_header_t *header = headerMapping.GetMemory<ipc_header_t>();
			
			KernReturn<void> result;

			switch(arguments->mode)
			{
				case IPC_READ:
				{
					Message *message = Message::Alloc()->Init(header);
					result = Read(message);
					message->Release();
					
					break;
				}

				case IPC_WRITE:
				{
					Message *message = Message::Alloc()->Init(header);
					result = Write(message);
					message->Release();

					break;
				}
			}

			if(!result.IsValid())
				return result.GetError();

			return KERN_SUCCESS;
		}
	}

	KernReturn<void> Syscall_IPCInit()
	{
		SetSyscallHandler(SYS_IPC_TaskPort, &IPC::Syscall_IPCTaskPort, sizeof(IPC::IPCPortCallArgs), false);
		SetSyscallHandler(SYS_IPC_ThreadPort, &IPC::Syscall_IPCThreadPort, sizeof(IPC::IPCPortCallArgs), false);

		SetSyscallHandler(SYS_IPC_Message, &IPC::Syscall_IPCMessage, sizeof(IPC::IPCReadWriteArgs), false);

		return ErrorNone;
	}
}
