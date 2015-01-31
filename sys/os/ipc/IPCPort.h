//
//  IPCPort.h
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

#ifndef _IPCPORT_H_
#define _IPCPORT_H_

#include <prefix.h>
#include <kern/kern_return.h>
#include <libc/stdint.h>
#include <libc/stdbool.h>
#include <libc/sys/spinlock.h>
#include <libc/ipc/ipc_port.h>
#include <libcpp/bitfield.h>
#include <objects/IOObject.h>
#include <objects/IOString.h>
#include <objects/IOArray.h>
#include <objects/IOSet.h>
#include "IPCMessage.h"

namespace OS
{
	class Task;

	namespace IPC
	{
		class System;
		class PortRight;

		class Port : public IO::Object
		{
		public:
			friend class System;
			friend class PortRight;

			struct Rights : cpp::bitfield<uint32_t>
			{
				Rights() = default;
				Rights(int value) :
					bitfield(value)
				{}

				enum
				{
					Any = (1 << 0), // No special rights required to send to that port
					Inherited = (1 << 1), // Only child tasks and especially granted ports can send
					Receive = (1 << 2) // Port is a receiving port, otherwise it's a sending port
				};
			};

			void Dealloc() override;
			bool IsPortRight() const;

			uint16_t GetName() const { return _name; }
			ipc_port_t GetPortName() const { return _portName; }
			System *GetSystem() const { return _system; }
			Rights GetRights() const { return _rights; }

			void PushMessage(Message *message);

			Message *PeekMessage();
			void PopMessage();

		protected:
			Port *Init(System *system, uint16_t name, Rights rights);
			Port *InitAsPortRight(Port *port, uint16_t name);

		private:
			Rights _rights;

			uint16_t _name;
			ipc_port_t _portName;
			
			IO::Array *_queue;
			System *_system;
			
			IODeclareMeta(Port)
		};
	}
}

#endif /* _IPCPORT_H_ */
