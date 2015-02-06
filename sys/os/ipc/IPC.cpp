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
		System *LookupSystem(ipc_port_t name)
		{
			Task *task = Scheduler::GetTaskWithPID(IPCGetPID(name));
			if(!task)
				return nullptr;

			System *system = task->GetIPCSystem(IPCGetSystem(name));
			if(!system)
				return nullptr;

			return system;
		}

		
		IO::StrongRef<Port> LookupPort(ipc_port_t name, bool lockSystem)
		{
			if(IPCIsPortRight(name))
				return nullptr;

			System *system = LookupSystem(name);
			if(!system)
				return nullptr;

			system->Lock();

			uint16_t portName = IPCGetName(name);

			IO::StrongRef<Port> port = (IPCIsPortRight(name)) ? system->GetPortRight(portName) : system->GetPort(portName);
			if(!port)
			{
				system->Unlock();
				return nullptr;
			}

			if(!lockSystem)
				system->Unlock();

			return port;
		}

		KernReturn<void> ValidateMessage(Port *port, Message *message, Task *sender)
		{
			ipc_port_t sendPortName = message->GetSender();
			IO::StrongRef<Port> sendPort = nullptr;

			if(sendPortName != IPC_PORT_NULL)
			{
				pid_t sendPid = IPCGetPID(sendPortName);

				if(sendPid != sender->GetPid())
					return Error(KERN_ACCESS_VIOLATION);

				// Check if this is a send port
				sendPort = LookupPort(sendPortName, false);
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

			// Port right validation
			if(sendPort->IsPortRight())
			{
				PortRight *right = static_cast<PortRight *>(sendPort.Load());

				if(right->GetHolder() == sender && right->GetPort() == port)
					return ErrorNone;
			}

			return Error(KERN_ACCESS_VIOLATION);
		}


		KernReturn<void> Write(Message *message)
		{
			ipc_port_t receiverPortName = message->GetReceiver();
			System *system = LookupSystem(receiverPortName);

			IO::StrongRef<Port> port = LookupPort(receiverPortName, true);
			if(!port)
				return Error(KERN_INVALID_ARGUMENT);

			KernReturn<void> result = ValidateMessage(port, message, Scheduler::GetActiveTask());
			if(!result.IsValid())
			{
				system->Unlock();
				return result;
			}

			Message *copy = Message::Alloc()->InitAsCopy(message);
			port->PushMessage(copy);
			copy->Release();

			if(port->IsPortRight())
			{
				PortRight *right = static_cast<PortRight *>(port.Load());

				if(right->IsOneTime())
					system->RelinquishPortRight(right);
			}

			system->Unlock();
			Wakeup(port);

			return ErrorNone;
		}

		KernReturn<void> Read(Message *message)
		{
			if(IPCIsPortRight(message->GetReceiver()))
				return Error(KERN_IPC_NO_RECEIVER);

			IO::StrongRef<Port> port = LookupPort(message->GetReceiver(), true);
			if(!port)
				return Error(KERN_INVALID_ARGUMENT);

			// Validate that we can even read from that port
			Task *receiver = Scheduler::GetActiveTask();
			if(receiver->GetPid() != (pid_t)IPCGetPID(message->GetReceiver()))
				return Error(KERN_ACCESS_VIOLATION);

			if(!(port->GetRights() & Port::Rights::Receive))
				return Error(KERN_IPC_NO_RECEIVER);

			ipc_header_t *header = message->GetHeader();
			System *system = port->GetSystem();

			goto readMessage;

		readMessageRetry:
			system->Lock();

		readMessage:
			Message *queuedMessage = port->PeekMessage();
			if(!queuedMessage)
			{
				if(header->flags & IPC_HEADER_FLAG_BLOCK)
				{
					WaitWithCallback(port.Load(), [system] {
						system->Unlock();
					});

					goto readMessageRetry;
				}

				system->Unlock();
				return Error(KERN_RESOURCE_EXHAUSTED);
			}
			
			ipc_header_t *queuedHeader = queuedMessage->GetHeader();

			header->realSize = queuedHeader->realSize;
			if(queuedHeader->size > header->size)
			{
				system->Unlock();
				return Error(KERN_NO_MEMORY);
			}

			// Retain the message and unblock the port before copying it
			queuedMessage->Retain();

			port->PopMessage();
			system->Unlock();

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
	
	KernReturn<void> IPCInit()
	{
		return ErrorNone;
	}
}
