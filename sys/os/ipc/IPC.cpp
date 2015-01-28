//
//  IPC.cpp
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

#include <kern/kprintf.h>
#include <os/scheduler/scheduler.h>
#include <os/waitqueue.h>
#include <libc/string.h>
#include "IPC.h"

namespace OS
{
	namespace IPC
	{
		KernReturn<IO::StrongRef<Port>> LookupPort(ipc_port_t name)
		{
			Task *task = Scheduler::GetTaskWithPID(IPCGetPID(name));
			if(!task)
				return Error(KERN_INVALID_ARGUMENT);

			System *system = task->GetIPCSystem(IPCGetSystem(name));
			if(!system)
				return Error(KERN_INVALID_ARGUMENT);

			IO::StrongRef<Port> port = system->GetPort(IPCGetPort(name));
			if(!port)
				return Error(KERN_INVALID_ARGUMENT);

			return port;
		}

		KernReturn<void> ValidateMessage(Message *message, Task *sender)
		{
			ipc_port_t sendPortName = message->GetSender();
			IO::StrongRef<Port> sendPort = nullptr;

			if(sendPortName != IPC_PORT_NULL)
			{
				pid_t sendPid = IPCGetPID(sendPortName);

				if(sendPid != sender->GetPid())
					return Error(KERN_ACCESS_VIOLATION);

				// Check if this is a send port
				sendPort = LookupPort(sendPortName);
				if(!sendPort)
					return Error(KERN_INVALID_ARGUMENT);

				if((sendPort->GetRights() & Port::Rights::Receive))
					return Error(KERN_IPC_NO_SENDER);
			}
			else
			{
				// If no sender is set, the Task port is used instead
				ipc_header_t *header = message->GetHeader();
				sendPort = sender->GetTaskPort();

				header->sender = sendPort->GetPortName();
			}


			// Make sure the receiver grants us rights to send to it
			IO::StrongRef<Port> port = LookupPort(message->GetReceiver());
			if(!port)
				return Error(KERN_INVALID_ARGUMENT);

			Port::Rights rights = port->GetRights();
			pid_t pid = IPCGetPID(message->GetReceiver());

			if(!(rights & IPC::Port::Rights::Receive))
				return Error(KERN_IPC_NO_RECEIVER);

			// If anyone is allowed to send, or if we send to ourselves, everything is A-okay
			if((rights & IPC::Port::Rights::Any) || pid == sender->GetPid())
				return ErrorNone;

			if((rights & IPC::Port::Rights::Inherited))
			{
				Task *parent = sender->GetParent();

				if(parent && parent->GetPid() == pid)
					return ErrorNone;
			}

			// Check if the task or port is granted special send rights
			port->Lock();

			if(port->HasTaskRight(sender))
			{
				port->Unlock();
				return ErrorNone;
			}

			if(port->HasPortRight(sendPort))
			{
				port->Unlock();
				return ErrorNone;
			}

			port->Unlock();

			return Error(KERN_ACCESS_VIOLATION);
		}




		KernReturn<void> Write(Message *message)
		{
			KernReturn<void> result = ValidateMessage(message, Scheduler::GetActiveTask());
			if(!result.IsValid())
			{
				kprintf("Validation failed!");
				return result;
			}

			IO::StrongRef<Port> port = LookupPort(message->GetReceiver());
			if(!port)
				return Error(KERN_INVALID_ARGUMENT);

			Message *copy = Message::Alloc()->InitAsCopy(message);

			port->Lock();
			port->PushMessage(copy);
			port->Unlock();

			copy->Release();
			Wakeup(port);

			return ErrorNone;
		}

		KernReturn<void> Read(Message *message)
		{
			IO::StrongRef<Port> port = LookupPort(message->GetReceiver());
			if(!port)
				return Error(KERN_INVALID_ARGUMENT);

			// Validate that we can even read from that port
			Task *receiver = Scheduler::GetActiveTask();
			if(receiver->GetPid() != (pid_t)IPCGetPID(message->GetReceiver()))
				return Error(KERN_ACCESS_VIOLATION);

			if(!(port->GetRights() & Port::Rights::Receive))
				return Error(KERN_IPC_NO_RECEIVER);

			ipc_header_t *header = message->GetHeader();

		loadMessage:
			port->Lock();

			Message *queuedMessage = port->PeekMessage();
			if(!queuedMessage)
			{
				if(header->flags & IPC_HEADER_FLAG_BLOCK)
				{
					WaitWithCallback(port.Load(), [port] {
						port->Unlock();
					});

					goto loadMessage;
				}

				port->Unlock();
				return Error(KERN_RESOURCE_EXHAUSTED);
			}
			
			ipc_header_t *queuedHeader = queuedMessage->GetHeader();

			header->realSize = queuedHeader->realSize;
			if(queuedHeader->size > header->size)
			{
				port->Unlock();
				return Error(KERN_NO_MEMORY);
			}

			// Retain the message and unblock the port before copying it
			queuedMessage->Retain();

			port->PopMessage();
			port->Unlock();

			// Copy the message content
			header->sender = queuedHeader->sender;
			header->sequence = queuedHeader->sequence;

			const uint8_t *source = IPC_GET_DATA(queuedHeader);
			uint8_t *target = IPC_GET_DATA(header);

			memcpy(target, source, header->size);
			queuedMessage->Release();

			return ErrorNone;
		}
	}

	KernReturn<void> Syscall_IPCInit();

	KernReturn<void> IPCInit()
	{
		return Syscall_IPCInit();
	}
}
