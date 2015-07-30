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
#include <libio/IOObject.h>
#include <libc/ipc/ipc_message.h>

namespace OS
{
	namespace IPC
	{
		class Message : public IO::Object
		{
		public:
			Message *Init(ipc_header_t *header);
			Message *InitAsCopy(const Message *other);

			void Dealloc() override;

			ipc_header_t *GetHeader() const { return _header; }
			ipc_port_t GetPort() const { return _header->port; }

			template<class T>
			const T *GetData() const { return reinterpret_cast<const T *>(IPC_GET_DATA(_header)); }

		private:
			ipc_header_t *_header;
			bool _ownsData;

			IODeclareMeta(Message)
		};
	}
}

#endif /* _IPCMESSAGE_H_ */
