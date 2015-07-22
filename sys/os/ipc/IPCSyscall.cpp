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
			*port = thread->GetTask()->GetTaskPort()->GetName();

			return KERN_SUCCESS;
		}

		KernReturn<uint32_t> Syscall_IPCThreadPort(Thread *thread, IPCPortCallArgs *arguments)
		{
			OS::SyscallScopedMapping portMapping(thread->GetTask(), arguments->port, sizeof(ipc_port_t));

			ipc_port_t *port = portMapping.GetMemory<ipc_port_t>();
			*port = thread->GetThreadPort()->GetName();

			return KERN_SUCCESS;
		}

		KernReturn<uint32_t> Syscall_IPCMessage(Thread *thread, IPCReadWriteArgs *arguments)
		{
			if(arguments->size == 0 || (arguments->mode != IPC_WRITE && arguments->mode != IPC_READ))
				return KERN_INVALID_ARGUMENT;

			Space *space = thread->GetTask()->GetIPCSpace();

			OS::SyscallScopedMapping headerMapping(thread->GetTask(), arguments->header, arguments->size + sizeof(ipc_header_t));
			ipc_header_t *header = headerMapping.GetMemory<ipc_header_t>();
			
			KernReturn<void> result;

			space->Lock();

			switch(arguments->mode)
			{
				case IPC_READ:
				{
					Message *message = Message::Alloc()->Init(header);
					result = space->Read(message);
					message->Release();
					
					break;
				}

				case IPC_WRITE:
				{
					Message *message = Message::Alloc()->Init(header);
					result = space->Write(message);
					message->Release();

					break;
				}
			}

			space->Unlock();

			if(!result.IsValid())
				return result.GetError();

			return KERN_SUCCESS;
		}

		KernReturn<uint32_t> Syscall_IPCAllocatePort(Thread *thread, IPCPortCallArgs *arguments)
		{
			OS::SyscallScopedMapping portMapping(thread->GetTask(), arguments->port, sizeof(ipc_port_t));
			ipc_port_t *port = portMapping.GetMemory<ipc_port_t>();

			Task *task = thread->GetTask();
			Space *space = task->GetIPCSpace();

			space->Lock();

			KernReturn<Port *> result = space->AllocateReceivePort();
			if(!result.IsValid())
			{
				space->Unlock();

				return result.GetError();
			}

			*port = result->GetName();
			space->Unlock();

			return KERN_SUCCESS;
		}

		KernReturn<uint32_t> Syscall_IPCGetSpecialPort(Thread *thread, IPCSpecialPortArgs *arguments)
		{
			if(arguments->port < 0 || arguments->port >= __IPC_SPECIAL_PORT_MAX)
				return Error(KERN_INVALID_ARGUMENT);

			OS::SyscallScopedMapping portMapping(thread->GetTask(), arguments->result, sizeof(ipc_port_t));

			ipc_port_t *result = portMapping.GetMemory<ipc_port_t>();
			Port *port = thread->GetTask()->GetSpecialPort(arguments->port);

			*result = port->GetName();

			return ErrorNone;
		}

		KernReturn<uint32_t> Syscall_IPCDeallocatePort(Thread *thread, IPCDeallcoatePortArgs *arguments)
		{
			Task *task = thread->GetTask();
			Space *space = task->GetIPCSpace();

			space->Lock();

			Port *port = space->GetPortWithName(arguments->port);
			if(!port)
			{
				space->Unlock();
				return Error(KERN_INVALID_ARGUMENT);
			}

			space->DeallocatePort(port);
			space->Unlock();

			return KERN_SUCCESS;
		}

		KernReturn<uint32_t> Syscall_IPCTaskSpace(Thread *thread, IPCTaskSpaceCallArgs *arguments)
		{
			OS::SyscallScopedMapping portMapping(thread->GetTask(), arguments->space, sizeof(ipc_space_t));
			ipc_space_t *space = portMapping.GetMemory<ipc_space_t>();

			Task *task = Scheduler::GetScheduler()->GetTaskWithPID(arguments->pid);
			if(!task)
				return Error(KERN_INVALID_ARGUMENT);

			*space = task->GetIPCSpace()->GetName();

			return KERN_SUCCESS;
		}

		KernReturn<uint32_t> Syscall_IPCInsertPort(Thread *thread, IPCInsertPortArgs *arguments)
		{
			if(arguments->right <= IPC_PORT_RIGHT_RECEIVE || arguments->right > IPC_PORT_RIGHT_SEND_ONCE)
				return Error(KERN_INVALID_ARGUMENT);

			IO::StrongRef<Space> space = Space::GetSpaceWithName(arguments->space);
			if(!space)
				return Error(KERN_INVALID_ARGUMENT);

			Space *source = thread->GetTask()->GetIPCSpace();
			bool differentLocks = (source != space);

			Space *space1 = (source < space.Load()) ? source : space.Load();
			Space *space2 = (source < space.Load()) ? space.Load() : source;

			space1->Lock();

			if(differentLocks)
				space2->Lock();

			Port *port = source->GetPortWithName(arguments->port);
			if(!port || port->GetRight() != Port::Right::Receive)
			{
				space2->Unlock();

				if(differentLocks)
					space1->Unlock();

				return Error(KERN_INVALID_ARGUMENT);
			}

			Port::Right right;

			switch(arguments->right)
			{
				case IPC_PORT_RIGHT_SEND:
					right = Port::Right::Send;
					break;
				case IPC_PORT_RIGHT_SEND_ONCE:
					right = Port::Right::SendOnce;
					break;
			}

			KernReturn<Port *> target = space->AllocateSendPort(port, right, arguments->target);

			space2->Unlock();

			if(differentLocks)
				space1->Unlock();

			if(!target.IsValid())
				return target.GetError();

			return KERN_SUCCESS;
		}
	}
}
