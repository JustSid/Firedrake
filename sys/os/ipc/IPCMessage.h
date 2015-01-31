//
//  IPCMessage.h
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

#ifndef _IPCMESSAGE_H_
#define _IPCMESSAGE_H_

#include <prefix.h>
#include <kern/kern_return.h>
#include <machine/memory/virtual.h>
#include <objects/IOObject.h>
#include <libc/ipc/ipc_message.h>

#define IPCCreatePortName(pid, system, port) \
	((static_cast<ipc_port_t>((pid)) << 32) | ((system) << 16) | (port & 0x7fff))
#define IPCCreatePortRight(pid, system, port) \
	((static_cast<ipc_port_t>((pid)) << 32) | ((system) << 16) | (1 << 15) | (port & 0x7fff))

#define IPCGetPID(name) \
	(pid_t)((name) >> 32)
#define IPCGetSystem(name) \
	(((name) >> 16) & 0xffff)
#define IPCGetName(name) \
	((name) & 0x7fff)
#define IPCIsPortRight(name) \
	(bool)((name) & (1 << 15))

namespace OS
{
	namespace IPC
	{
		class System;

		class Message : public IO::Object
		{
		public:
			Message *Init(ipc_header_t *header);
			Message *InitAsCopy(const Message *other);

			void Dealloc() override;

			ipc_header_t *GetHeader() const { return _header; }

			ipc_port_t GetSender() const { return _header->sender; }
			ipc_port_t GetReceiver() const { return _header->receiver; }

		private:
			ipc_header_t *_header;
			void *_buffer;
			bool _ownsData;

			IODeclareMeta(Message)
		};
	}
}

#endif /* _IPCMESSAGE_H_ */
