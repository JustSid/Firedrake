//
//  bootstrapserver.cpp
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

#include <prefix.h>
#include <libcpp/algorithm.h>
#include <kern/kprintf.h>
#include <libc/string.h>
#include <libio/core/IOString.h>
#include <libio/core/IONumber.h>
#include <libio/core/IODictionary.h>
#include <os/scheduler/scheduler.h>
#include <os/ipc/IPC.h>

namespace OS
{
	extern IPC::Port *bootstrapPort;
	static IO::Dictionary *_bootstrapPorts;

	void BootstrapServerRegister(ipc_header_t *header)
	{
		char buffer[255];

		size_t size = std::min(sizeof(buffer), static_cast<size_t>(header->size));
		size_t length = strlcpy(buffer, reinterpret_cast<char *>(IPC_GET_DATA(header)), size);
		if(length == 0)
			return;

		IO::String *string = IO::String::Alloc()->InitWithCString(buffer);
		IO::Number *portNumber = IO::Number::Alloc()->InitWithUint32(header->port);

		_bootstrapPorts->SetObjectForKey(portNumber, string);

		string->Release();
		portNumber->Release();
	}

	void BootstrapServerUnregister(ipc_header_t *header)
	{
		char buffer[255];

		size_t size = std::min(sizeof(buffer), static_cast<size_t>(header->size));
		size_t length = strlcpy(buffer, reinterpret_cast<char *>(IPC_GET_DATA(header)), size);
		if(length == 0)
			return;

		IO::String *string = IO::String::Alloc()->InitWithCString(buffer);
		_bootstrapPorts->RemoveObjectForKey(string);
		string->Release();
	}


	void BootstrapServerLookup(IPC::Space *space, ipc_header_t *inHeader)
	{
		char buffer[255];

		size_t size = std::min(sizeof(buffer), static_cast<size_t>(inHeader->size));
		size_t length = strlcpy(buffer, reinterpret_cast<char *>(IPC_GET_DATA(inHeader)), size);
		if(length == 0)
			return;

		IO::String *string = IO::String::Alloc()->InitWithCString(buffer, false);
		IO::Number *portNumber = _bootstrapPorts->GetObjectForKey<IO::Number>(string);

		string->Release();

		char replyBuffer[sizeof(ipc_return_t) + sizeof(ipc_header_t)];
		ipc_header_t *header = (ipc_header_t *)replyBuffer;
		
		header->port = inHeader->port;
		header->flags = 0;
		header->size = sizeof(ipc_return_t);

		if(portNumber)
		{
			ipc_return_t *data = (ipc_return_t *)IPC_GET_DATA(header);
			*data = KERN_SUCCESS;

			header->flags = IPC_HEADER_FLAG_RESPONSE | IPC_HEADER_FLAG_RESPONSE_BITS(IPC_MESSAGE_RIGHT_COPY_SEND);
			header->reply = portNumber->GetUint32Value();
		}
		else
		{
			ipc_return_t *data = (ipc_return_t *)IPC_GET_DATA(header);
			*data = KERN_RESOURCE_NOT_FOUND;
		}


		IPC::Message *message = IPC::Message::Alloc()->Init(header);

		space->Lock();
		KernReturn<void> result = space->Write(message);
		if(!result.IsValid())
		{
			kprintf("Result: %d\n", result.GetError().GetCode());
		}
		space->Unlock();

		message->Release();
	}

	void BootstrapServerThread()
	{
		_bootstrapPorts = IO::Dictionary::Alloc()->Init();

		Task *self = Scheduler::GetScheduler()->GetActiveTask();
		IPC::Space *space = self->GetIPCSpace();

		while(1)
		{
			size_t bufferSize = 1024;

			void *buffer = kalloc(bufferSize);
			ipc_header_t *header = reinterpret_cast<ipc_header_t *>(buffer);

			IPC::Message *message = IPC::Message::Alloc()->Init(header);

			while(1)
			{
				header->port = bootstrapPort->GetName();
				header->flags = IPC_HEADER_FLAG_BLOCK;
				header->size = bufferSize - sizeof(ipc_header_t);

				space->Lock();
				KernReturn<void> result = space->Read(message);
				space->Unlock();

				if(result.IsValid())
				{
					switch(header->id)
					{
						case 0: // ipc_bootstrap_register
							BootstrapServerRegister(header);
							break;
						case 1: // ipc_bootstrap_lookup
							BootstrapServerLookup(space, header);
							break;
						case 2: // ipc_bootstrap_unregister,
							BootstrapServerUnregister(header);
							break;

						default:
							kprintf("Invalid bootstrap request %lu\n", header->id);
							break;
					}
				}
				else
				{
					switch(result.GetError().GetCode())
					{
						case KERN_NO_MEMORY:
						{
							// Expand the buffer to allow reading the whole message
							bufferSize = message->GetHeader()->size + sizeof(ipc_header_t);
							kfree(buffer);

							buffer = kalloc(bufferSize);

							header = reinterpret_cast<ipc_header_t *>(buffer);

							message->Release();
							message = IPC::Message::Alloc()->Init(header);

							break;
						}

						default:
							break;
					}
				}
			}
		}
	}
}
