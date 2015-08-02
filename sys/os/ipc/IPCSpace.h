//
//  IPCSpace.h
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

#ifndef _IPCSPACE_H_
#define _IPCSPACE_H_

#include <prefix.h>
#include <libio/core/IOObject.h>
#include <libio/core/IODictionary.h>
#include <os/locks/mutex.h>
#include <libc/ipc/ipc_types.h>
#include "IPCPort.h"

namespace OS
{
	class Task;

	namespace IPC
	{
		class Message;

		class Space : public IO::Object
		{
		public:
			Space *Init();
			void Dealloc() override;

			static IO::StrongRef<Space> GetSpaceWithName(ipc_space_t name);

			KernReturn<Port *> AllocateReceivePort(); // Creates a new port with receive rights
			KernReturn<Port *> AllocateSendPort(Port *target, Port::Right right, ipc_port_t name); // Right must be either Send or SendOnce
			KernReturn<Port *> AllocateCallbackPort(Port::Callback callback);

			void DeallocatePort(Port *port);

			Port *GetPortWithName(ipc_port_t name) const;

			/** Must *both* be called with lock being held **/
			KernReturn<void> Write(Message *message);
			KernReturn<void> Read(Message *message);

			ipc_space_t GetName() const { return _name; }
			Task *GetTask() const { return _task; }

			void Lock();
			void Unlock();

		private:
			ipc_space_t _name;
			IO::Dictionary *_ports;
			Task *_task;
			Mutex _lock;
			ipc_port_t _portNames;

			IODeclareMeta(Space)
		};
	}

	KernReturn<void> IPCInit();
}

#endif /* _IPCSPACE_H_ */
