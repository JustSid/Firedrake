//
//  IPCSpace.cpp
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

#include <libio/IONumber.h>
#include <os/waitqueue.h>
#include <kern/kprintf.h>
#include "IPCSpace.h"
#include "IPCMessage.h"

namespace OS
{
	namespace IPC
	{
		IODefineMeta(Space, IO::Object)

		static IO::Dictionary *_spaceMap;
		static Mutex _spaceLock;
		static std::atomic<ipc_space_t> _spaceName;

		Space *Space::Init()
		{
			if(!IO::Object::Init())
				return nullptr;

			_ports = IO::Dictionary::Alloc()->Init();
			_name = _spaceName ++;
			_portNames = 1;

			{
				_spaceLock.Lock();

				IO::StrongRef<IO::Number> lookup(IOTransferRef(IO::Number::Alloc()->InitWithUint32(_name)));
				_spaceMap->SetObjectForKey(this, lookup);

				_spaceLock.Unlock();
			}

			return this;
		}
		void Space::Dealloc()
		{
			IO::Object::Dealloc();
		}


		IO::StrongRef<Space> Space::GetSpaceWithName(ipc_space_t name)
		{
			_spaceLock.Lock();

			IO::StrongRef<IO::Number> lookup(IOTransferRef(IO::Number::Alloc()->InitWithUint32(name)));
			IO::StrongRef<Space> space = _spaceMap->GetObjectForKey<Space>(lookup);

			_spaceLock.Unlock();

			return space;
		}

		void Space::Lock()
		{
			_lock.Lock();
		}
		void Space::Unlock()
		{
			_lock.Unlock();
		}

		KernReturn<Port *> Space::AllocateReceivePort()
		{
			while(1)
			{
				ipc_port_t name = _portNames ++;
				IO::StrongRef<IO::Number> lookup(IOTransferRef(IO::Number::Alloc()->InitWithUint32(name)));

				if(!_spaceMap->GetObjectForKey<Port>(lookup))
				{
					Port *port = Port::Alloc()->InitWithReceiveRight(this, name);
					if(!port)
						return Error(KERN_NO_MEMORY);

					_ports->SetObjectForKey(port, lookup);
					return port;
				}
			}
		}

		KernReturn<Port *> Space::AllocateSendPort(Port *target, Port::Right right, ipc_port_t name)
		{
			IOAssert(target && target->GetRight() == Port::Right::Receive, "Target mustn't be NULL and must be a receive right");
			IOAssert(right == Port::Right::Send || right == Port::Right::SendOnce, "Right must be either Send or SendOnce");


			if(name == IPC_PORT_NULL)
			{
				while(1)
				{
					ipc_port_t name = _portNames ++;
					IO::StrongRef<IO::Number> lookup(IOTransferRef(IO::Number::Alloc()->InitWithUint32(name)));

					if(!_spaceMap->GetObjectForKey<Port>(lookup))
					{
						Port *port = Port::Alloc()->InitWithSendRight(this, name, right, target);
						if(!port)
							return Error(KERN_NO_MEMORY);

						_ports->SetObjectForKey(port, lookup);
						return port;
					}
				}
			}
			else
			{
				IO::StrongRef<IO::Number> lookup(IOTransferRef(IO::Number::Alloc()->InitWithUint32(name)));

				if(_spaceMap->GetObjectForKey<Port>(lookup))
					return Error(KERN_RESOURCE_EXISTS);

				Port *port = Port::Alloc()->InitWithSendRight(this, name, right, target);
				if(!port)
					return Error(KERN_NO_MEMORY);

				_ports->SetObjectForKey(port, lookup);

				return port;
			}
		}

		KernReturn<Port *> Space::AllocateCallbackPort(Port::Callback callback)
		{
			while(1)
			{
				ipc_port_t name = _portNames ++;
				IO::StrongRef<IO::Number> lookup(IOTransferRef(IO::Number::Alloc()->InitWithUint32(name)));

				if(!_spaceMap->GetObjectForKey<Port>(lookup))
				{
					Port *port = Port::Alloc()->InithWithCallback(this, name, callback);
					if(!port)
						return Error(KERN_NO_MEMORY);

					_ports->SetObjectForKey(port, lookup);
					return port;
				}
			}
		}

		void Space::DeallocatePort(Port *port)
		{
			port->MarkDead();

			IO::StrongRef<IO::Number> lookup(IOTransferRef(IO::Number::Alloc()->InitWithUint32(port->GetName())));
			_ports->RemoveObjectForKey(lookup);
		}

		Port *Space::GetPortWithName(ipc_port_t name) const
		{
			IO::StrongRef<IO::Number> lookup(IOTransferRef(IO::Number::Alloc()->InitWithUint32(name)));
			return _ports->GetObjectForKey<Port>(lookup);
		}

		KernReturn<void> Space::Write(Message *message)
		{
			Port *sender = GetPortWithName(message->GetPort());

			if(!sender || sender->GetRight() == Port::Right::Receive)
				return Error(KERN_INVALID_ARGUMENT);

			Port *target = sender->GetTarget();
			if(!target || target->IsDead())
				return Error(KERN_IPC_NO_RECEIVER);

			Space *targetSpace = target->GetSpace();

			if(targetSpace != this)
				targetSpace->Lock();

			Message *copy = Message::Alloc()->InitAsCopy(message);

			if(target->IsDead())
				return Error(KERN_IPC_NO_RECEIVER);

			ipc_header_t *header = copy->GetHeader();
			header->port = target->GetName();

			if(header->flags & IPC_HEADER_FLAG_RESPONSE)
			{
				Port *replyPort = GetPortWithName(header->reply);
				header->reply = IPC_PORT_DEAD;

				if(replyPort)
				{
					if(replyPort->GetRight() != Port::Right::Receive)
					{
						replyPort = replyPort->GetTarget();

						if(!replyPort || replyPort->IsDead())
						{
							if(targetSpace != this)
								targetSpace->Unlock();

							copy->Release();

							return Error(KERN_IPC_NO_RECEIVER);
						}
					}

					Port::Right right = Port::Right::Send;
					int responseRight = IPC_HEADER_FLAG_GET_RESPONSE_BITS(header->flags);

					if(responseRight == IPC_MESSAGE_RIGHT_MOVE_SEND_ONCE || responseRight == IPC_MESSAGE_RIGHT_COPY_SEND_ONCE)
						right = Port::Right::SendOnce;

					KernReturn<Port *> mapped = targetSpace->AllocateSendPort(replyPort, right, IPC_PORT_NULL);
					if(mapped.IsValid())
					{
						header->reply = mapped->GetName();

						if(responseRight == IPC_MESSAGE_RIGHT_MOVE_SEND || responseRight == IPC_MESSAGE_RIGHT_MOVE_SEND_ONCE)
							DeallocatePort(replyPort);
					}
					else
					{
						if(targetSpace != this)
							targetSpace->Unlock();

						copy->Release();
						return mapped.GetError();
					}
				}
			}

			target->PushMessage(copy);
			copy->Release();

			if(targetSpace != this)
				targetSpace->Unlock();

			if(sender->GetRight() == Port::Right::SendOnce)
				DeallocatePort(sender);

			Wakeup(target);

			return ErrorNone;
		}

		KernReturn<void> Space::Read(Message *message)
		{
			IO::StrongRef<Port> receiver = GetPortWithName(message->GetPort());

			if(!receiver || receiver->GetRight() != Port::Right::Receive)
				return Error(KERN_INVALID_ARGUMENT);

			ipc_header_t *header = message->GetHeader();

			goto readMessage;

		readMessageRetry:
			Lock();

		readMessage:
			Message *queuedMessage = receiver->PeekMessage();
			if(!queuedMessage)
			{
				if(header->flags & IPC_HEADER_FLAG_BLOCK)
				{
					KernReturn<void> result = WaitWithCallback(receiver.Load(), [this] {
						Unlock();
					});

					if(!result.IsValid()) // No need to lock because the lambda is only performed when the wait succeeds
						return Error(KERN_TASK_RESTART);

					goto readMessageRetry;
				}

				return Error(KERN_RESOURCE_EXHAUSTED);
			}

			ipc_header_t *queuedHeader = queuedMessage->GetHeader();

			header->realSize = queuedHeader->realSize;

			if(queuedHeader->size > header->size)
				return Error(KERN_NO_MEMORY);

			// Prepare the message
			queuedMessage->Retain();
			receiver->PopMessage();

			header->id = queuedHeader->id;
			header->reply = queuedHeader->port;
			header->port = queuedHeader->reply;

			memcpy(IPC_GET_DATA(header), IPC_GET_DATA(queuedHeader), header->size);
			queuedMessage->Release();

			return ErrorNone;
		}
	}

	KernReturn<void> IPCInit()
	{
		IPC::_spaceMap = IO::Dictionary::Alloc()->Init();

		if(!IPC::_spaceMap)
			return Error(KERN_NO_MEMORY);

		return ErrorNone;
	}
}
