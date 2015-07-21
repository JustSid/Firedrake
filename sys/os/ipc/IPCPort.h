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
		class Space;

		class Port : public IO::Object
		{
		public:
			friend class Space;
			
			typedef void (*Callback)(Port *port, Message *message); 

			enum class Right
			{
				Receive,
				Send,
				SendOnce
			};

			enum class Type
			{
				Regular,
				Callback // A port that isn't actually attached to anything but rather invokes a kernel callback
			};

			void Dealloc() override;
			void MarkDead();

			bool IsDead() const { return _isDead; }

			ipc_port_t GetName() const { return _name; }
			Space *GetSpace() const { return _space; }
			Right GetRight() const { return _right; }
			Type GetType() const { return _type; }
			Port *GetTarget() const { return _targetPort; }

			void PushMessage(Message *message);

			Message *PeekMessage();
			void PopMessage();

		protected:
			Port *InitWithReceiveRight(Space *space, ipc_port_t name);
			Port *InitWithSendRight(Space *space, ipc_port_t name, Right right, Port *target);
			Port *InithWithCallback(Space *space, ipc_port_t name, Callback callback);

		private:
			Port *Init(Space *space, ipc_port_t port, Type type);

			bool _isDead;
			Right _right;
			Type _type;

			ipc_port_t _name;
			Space *_space;

			union
			{
				IO::Array *_queue;
				Port *_targetPort;
				Callback _callback;
			};
			
			IODeclareMeta(Port)
		};
	}
}

#endif /* _IPCPORT_H_ */
