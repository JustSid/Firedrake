//
//  IPCSystem.h
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

#ifndef _IPCSYSTEM_H_
#define _IPCSYSTEM_H_

#include <prefix.h>
#include <libc/sys/spinlock.h>
#include <objects/IOObject.h>
#include <objects/IOString.h>
#include <objects/IODictionary.h>
#include "IPCPort.h"

namespace OS
{
	class Task;

	namespace IPC
	{
		class System : public IO::Object
		{
		public:
			System *Init(Task *task, uint16_t name);
			void Dealloc() override;

			void Lock();
			void Unlock();

			uint16_t GetName() const { return _name; }
			Task *GetTask() const { return _task; }

			Port *AllocatePort(uint16_t name, Port::Rights rights);
			IO::StrongRef<Port> GetPort(uint16_t name);

		private:
			Task *_task;
			uint16_t _name;
			IO::Dictionary *_ports;
			spinlock_t _lock;

			IODeclareMeta(System)
		};
	}
}

#endif /* _IPCSYSTEM_H_ */
