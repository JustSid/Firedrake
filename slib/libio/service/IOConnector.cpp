//
//  IOConnector.cpp
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
#include "../core/IORuntime.h"
#include "IOConnector.h"

namespace IO
{
	IODefineMeta(Connector, Object)

	Connector *Connector::InitWithName(const char *name)
	{
		if(!Object::Init())
			return nullptr;

		char temp[255];
		strlcpy(temp, name, 255);

		_thread = nullptr;
		_port = IPC_PORT_NULL;
		_name = String::Alloc()->InitWithCString(temp);

		return this;
	}

	Connector *Connector::InitWithService(Service *service)
	{
		return Connector::InitWithName(service->GetClass()->GetFullname());
	}

	void Connector::Dealloc()
	{
		if(_thread)
		{
			_thread->Cancel();
			_thread->WaitForExit();
			_thread->Release();

			ipc_deallocate_port(_port);
		}

		_name->Release();

		Object::Dealloc();
	}


	void Connector::Publish()
	{
		IOAssert(!_published, "Connector must not be published already");

		ipc_allocate_port(&_port);
		_thread = Thread::Alloc()->InitWithEntry(&Connector::__ThreadEntry, this);
		_thread->Start();
		_published = true;
	}

	void Connector::AddDispatchEntry(uint32_t vector, DispatchCallback callback, bool hasResponse, void *context)
	{
		IOAssert(!_published, "Connector must not be published already");

		DispatchEntry entry;
		entry.callback = callback;
		entry.context = context;
		entry.vector = vector;
		entry.hasResponse = hasResponse;

		_dispatchEntries.push_back(entry);
	}

	bool Connector::DispatchMessage(uint32_t vector, uint8_t *buffer, size_t size, uint8_t *outResponse, size_t &outResponseSize) const
	{
		size_t count = _dispatchEntries.size();
		for(size_t i = 0; i < count; i ++)
		{
			const DispatchEntry &entry = _dispatchEntries.at(i);

			if(entry.vector == vector)
			{
				entry.callback(entry.context, buffer, size, outResponse, outResponseSize);
				return entry.hasResponse;
			}
		}

		return false;
	}

	void Connector::ThreadEntry()
	{
		// Register
		{
			struct
			{
				ipc_header_t header;
				char name[255];
			} message;

			message.header.flags = IPC_HEADER_FLAG_RESPONSE | IPC_HEADER_FLAG_GET_RESPONSE_BITS(IPC_MESSAGE_RIGHT_COPY_SEND);
			message.header.reply = _port;
			message.header.port = ipc_get_special_port(0);
			message.header.size = _name->GetLength();
			message.header.id = 0;

			strcpy(message.name, _name->GetCString());
			ipc_write(&message.header);
		}

		// Work loop
		uint8_t *receiveBuffer = new uint8_t[8192 + sizeof(ipc_header_t)];
		uint8_t *responseBuffer = new uint8_t[8192 + sizeof(ipc_header_t)];

		ipc_header_t *receive = reinterpret_cast<ipc_header_t *>(receiveBuffer);
		ipc_header_t *response = reinterpret_cast<ipc_header_t *>(responseBuffer);

		while(!_thread->IsCancelled())
		{
			receive->port = _port;
			receive->flags = IPC_HEADER_FLAG_BLOCK;
			receive->size = 8192;

			ipc_return_t result = ipc_read(receive);

			if(result == KERN_SUCCESS)
			{
				size_t responseSize = 8192;
				bool hasResponse = DispatchMessage(receive->id, receiveBuffer, receive->size, responseBuffer + sizeof(ipc_header_t), responseSize);

				if(hasResponse)
				{
					response->port = receive->port;
					response->flags = 0;
					response->size = responseSize;

					ipc_write(response);
				}
			}
		}

		delete[] receiveBuffer;
		delete[] responseBuffer;

		// Unregister
		{
			struct
			{
				ipc_header_t header;
				char name[255];
			} message;

			message.header.flags = 0;
			message.header.reply = _port;
			message.header.port = ipc_get_special_port(0);
			message.header.size = _name->GetLength();
			message.header.id = 1;

			strcpy(message.name, _name->GetCString());
			ipc_write(&message.header);
		}
	}

	void Connector::__ThreadEntry(__unused Thread *thread, void *context)
	{
		Connector *connector = reinterpret_cast<Connector *>(context);
		connector->ThreadEntry();
	}
}
