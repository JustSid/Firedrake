//
//  IPCPort.cpp
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

#include <os/scheduler/task.h>
#include <libio/IONumber.h>
#include <kern/kprintf.h>
#include "IPCPort.h"
#include "IPCMessage.h"

namespace OS
{
	namespace IPC
	{
		IODefineMeta(Port, IO::Object)

		Port *Port::Init(Space *space, ipc_port_t name, Type type)
		{
			if(!IO::Object::Init())
				return nullptr;

			_name = name;
			_space = space;
			_type = type;
			_isDead = false;
			_context = nullptr;

			return this;
		}

		Port *Port::InitWithReceiveRight(Space *space, ipc_port_t name)
		{
			if(!Port::Init(space, name, Type::Regular))
				return nullptr;

			_right = Right::Receive;
			_queue = IO::Array::Alloc()->Init();

			return this;
		}

		Port *Port::InitWithSendRight(Space *space, ipc_port_t name, Right right, Port *target)
		{
			IOAssert(right == Right::Send || right == Right::SendOnce, "Right must be a send or send once right");

			if(!Port::Init(space, name, Type::Regular))
				return nullptr;

			_right = right;
			_targetPort = IO::SafeRetain(target);

			return this;
		}

		Port *Port::InithWithCallback(Space *space, ipc_port_t name, Callback callback)
		{
			if(!Port::Init(space, name, Type::Callback))
				return nullptr;

			_right = Right::Receive;
			_callback = callback;

			return this;
		}

		void Port::Dealloc()
		{
			if(_type == Type::Regular)
			{
				switch(_right)
				{
					case Right::Receive:
						IO::SafeRelease(_queue);
						break;
					case Right::Send:
					case Right::SendOnce:
						IO::SafeRelease(_targetPort);
						break;
				}
			}

			IO::Object::Dealloc();
		}
		

		void Port::MarkDead()
		{
			_isDead = true;
			_right = static_cast<Right>(0xdeadbeef);

			IO::SafeRelease(_queue);
			IO::SafeRelease(_targetPort);
		}

		void Port::SetContext(void *context)
		{
			_context = context;
		}

		void Port::PushMessage(Message *msg)
		{
			IOAssert(_right == Right::Receive, "Port must be a receive port");
			
			switch(_type)
			{
				case Type::Regular:
					_queue->AddObject(msg);
					break;
				case Type::Callback:
				{
					ipc_header_t *header = msg->GetHeader();
					header->realSize = header->size; // Set the realSize properly

					_callback(this, msg);
					break;
				}
			}

			
		}
		Message *Port::PeekMessage()
		{
			IOAssert(_right == Right::Receive && _type == Type::Regular, "Port must be a regular receive port");

			if(_queue->GetCount() == 0)
				return nullptr;

			return _queue->GetFirstObject<Message>();
		}
		void Port::PopMessage()
		{
			IOAssert(_right == Right::Receive && _type == Type::Regular, "Port must be a regular receive port");
			_queue->RemoveObjectAtIndex(0);
		}
	}
}
