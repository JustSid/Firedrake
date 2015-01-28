//
//  IPCSystem.cpp
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
#include <objects/IONumber.h>
#include "IPCSystem.h"

namespace OS
{
	namespace IPC
	{
		IODefineMeta(System, IO::Object)

		System *System::Init(Task *task, uint16_t name)
		{
			if(!IO::Object::Init())
				return nullptr;

			_name = name;
			_ports = IO::Dictionary::Alloc()->Init();
			_lock = SPINLOCK_INIT;
			_task = task;

			return this;
		}

		void System::Dealloc()
		{
			IO::SafeRelease(_ports);
			IO::Object::Dealloc();
		}

		void System::Lock()
		{
			spinlock_lock(&_lock);
		}
		void System::Unlock()
		{
			spinlock_unlock(&_lock);
		}


		Port *System::AllocatePort(uint16_t name, Port::Rights rights)
		{
			Port *port = Port::Alloc()->Init(this, name, rights);
			IO::Number *lookup = IO::Number::Alloc()->InitWithUint16(name);

			Lock();

			if(_ports->GetObjectForKey(lookup))
			{
				port->Release();
				lookup->Release();

				Unlock();
				return nullptr;
			}

			_ports->SetObjectForKey(port, lookup);
			Unlock();

			lookup->Release();
			return port;
		}
		IO::StrongRef<Port> System::GetPort(uint16_t name)
		{
			IO::Number *lookup = IO::Number::Alloc()->InitWithUint16(name);

			Lock();
			IO::StrongRef<Port> port = _ports->GetObjectForKey<Port>(lookup);
			Unlock();

			lookup->Release();			
			return port;
		}
	}
}
