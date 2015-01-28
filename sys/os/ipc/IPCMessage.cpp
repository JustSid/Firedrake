//
//  IPCMessage.cpp
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

#include <kern/kalloc.h>
#include "IPCMessage.h"

namespace OS
{
	namespace IPC
	{
		IODefineMeta(Message, IO::Object)

		Message *Message::Init(ipc_header_t *header)
		{
			if(!IO::Object::Init())
				return nullptr;

			_header = header;
			_ownsData = false;

			return this;
		}

		Message *Message::InitAsCopy(const Message *other)
		{
			if(!IO::Object::Init())
				return nullptr;

			ipc_header_t *otherHeader = other->GetHeader();
			void *blob = kalloc(otherHeader->size + sizeof(ipc_header_t));
			
			memcpy(blob, otherHeader, otherHeader->size + sizeof(ipc_header_t));

			_header = reinterpret_cast<ipc_header_t *>(blob);
			_ownsData = true;

			return this;
		}

		void Message::Dealloc()
		{
			if(_ownsData)
				kfree(_header);

			IO::Object::Dealloc();
		}
	}
}
