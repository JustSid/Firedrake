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
#include <objects/IONumber.h>
#include "IPCPort.h"
#include "IPCMessage.h"

namespace OS
{
	namespace IPC
	{
		IODefineMeta(Port, IO::Object)

		Port *Port::Init(System *system, uint16_t name, Rights rights)
		{
			if(!IO::Object::Init())
				return nullptr;

			_rights = rights;
			_name = name;
			_system = system;
			_queue = IO::Array::Alloc()->Init();
			_portRights = IO::Set::Alloc()->Init();
			_taskRights = IO::Set::Alloc()->Init();
			_lock = SPINLOCK_INIT;

			return this;
		}

		void Port::Dealloc()
		{
			IO::SafeRelease(_queue);

			IO::SafeRelease(_portRights);
			IO::SafeRelease(_taskRights);

			IO::Object::Dealloc();
		}


		void Port::Lock()
		{
			spinlock_lock(&_lock);
		}
		void Port::Unlock()
		{
			spinlock_unlock(&_lock);
		}


		void Port::AddPortRight(Port *other)
		{
			IO::Number *number = IO::Number::Alloc()->InitWithUint64(other->GetPortName());
			_portRights->AddObject(number);
			number->Release();
		}
		void Port::AddTaskRight(Task *task)
		{
			IO::Number *number = IO::Number::Alloc()->InitWithUint32(task->GetPid());
			_taskRights->AddObject(number);
			number->Release();
		}


		void Port::RemovePortRight(Port *other)
		{
			IO::Number *number = IO::Number::Alloc()->InitWithUint64(other->GetPortName());
			_portRights->RemoveObject(number);
			number->Release();
		}
		void Port::RemoveTaskRight(Task *task)
		{
			IO::Number *number = IO::Number::Alloc()->InitWithUint32(task->GetPid());
			_taskRights->RemoveObject(number);
			number->Release();
		}


		bool Port::HasPortRight(Port *other)
		{
			IO::Number *number = IO::Number::Alloc()->InitWithUint64(other->GetPortName());
			bool result = _portRights->ContainsObject(number);
			number->Release();

			return result;
		}
		bool Port::HasTaskRight(Task *task)
		{
			IO::Number *number = IO::Number::Alloc()->InitWithUint32(task->GetPid());
			bool result = _taskRights->ContainsObject(number);
			number->Release();

			return result;
		}


		ipc_port_t Port::GetPortName() const
		{
			Task *task = _system->GetTask();
			return IPCCreateName(task->GetPid(), _system->GetName(), _name);
		}


		void Port::PushMessage(Message *msg)
		{
			_queue->AddObject(msg);
		}
		Message *Port::PeekMessage()
		{
			if(_queue->GetCount() == 0)
				return nullptr;

			return _queue->GetFirstObject<Message>();
		}
		void Port::PopMessage()
		{
			_queue->RemoveObjectAtIndex(0);
		}
	}
}
