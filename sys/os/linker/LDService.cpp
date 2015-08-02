//
//  LDService.cpp
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

#include <libc/ipc/ipc_message.h>
#include <libio/core/IONumber.h>
#include "LDService.h"

/**
 * LDService provides a lookup service similar to the bootstrap server
 * however it is purely for kernel <- userland connections, kernel extensions can register
 * themselves and user land processes can look up registered services. This is done through the
 * IOConnector class in the kernel and the ipc_connector_xxx functions on the client side
 */

namespace OS
{
	namespace LD
	{
		static IPC::Port *_kernelPort = nullptr;
		static IPC::Port *_kernelSendPort = nullptr;
		static IPC::Port *_userKernelPort = nullptr;

		// Protected by the IPC::Space lock
		static IO::Dictionary *_connectors = nullptr;

		IPC::Port *GetKernelPort()
		{
			return _kernelSendPort;
		}
		IPC::Port *GetUserlandKernelPort()
		{
			return _userKernelPort;
		}


		void HandleServiceMessage(IPC::Port *port, IPC::Message *msg)
		{
			if(port == _kernelPort)
			{
				struct
				{
					ipc_header_t header;
					char buffer[255];
				} *message;

				message = reinterpret_cast<decltype(message)>(msg->GetHeader());
				message->buffer[message->header.size] = '\0';

				switch(message->header.id)
				{
					case 0:
					{
						IO::String *name = IO::String::Alloc()->InitWithCString(message->buffer);
						IO::Number *portNum = IO::Number::Alloc()->InitWithUint32(message->header.reply);

						_connectors->SetObjectForKey(name, portNum);
						
						name->Release();
						portNum->Release();
						break;
					}

					case 1:
					{
						IO::String *name = IO::String::Alloc()->InitWithCString(message->buffer);
						_connectors->RemoveObjectForKey(name);
						name->Release();
						break;
					}
				}
			}
			else if(port == _userKernelPort)
			{
				struct
				{
					ipc_header_t header;
					char buffer[255];
				} *message;

				message = reinterpret_cast<decltype(message)>(msg->GetHeader());
				message->buffer[message->header.size] = '\0';

				IO::String *name = IO::String::Alloc()->InitWithCString(message->buffer);
				IO::Number *result = _connectors->GetObjectForKey<IO::Number>(name);
				name->Release();


				{
					struct
					{
						ipc_header_t header;
						ipc_return_t result;
					} reply;

					reply.header.port = message->header.port;
					reply.header.flags = 0;
					reply.header.size = sizeof(ipc_return_t);

					if(result)
					{
						reply.header.flags = IPC_HEADER_FLAG_RESPONSE | IPC_HEADER_FLAG_RESPONSE_BITS(IPC_MESSAGE_RIGHT_COPY_SEND);
						reply.header.reply = result->GetUint32Value();

						reply.result = KERN_SUCCESS;
					}
					else
					{
						reply.result = KERN_RESOURCE_NOT_FOUND;
					}

					IPC::Message *replyMessage = IPC::Message::Alloc()->Init(&reply.header);
					IPC::Space::GetKernelSpace()->Write(replyMessage);
				}
			}
		}
	}

	KernReturn<void> ServiceInit()
	{
		LD::_connectors = IO::Dictionary::Alloc()->Init();

		IPC::Space *space = IPC::Space::GetKernelSpace();

		space->Lock();

		LD::_kernelPort = space->AllocateCallbackPort(&LD::HandleServiceMessage);
		LD::_kernelSendPort = space->AllocateSendPort(LD::_kernelPort, IPC::Port::Right::Send, IPC_PORT_NULL);
		LD::_userKernelPort = space->AllocateCallbackPort(&LD::HandleServiceMessage);

		space->Unlock();

		return ErrorNone;
	}
}
