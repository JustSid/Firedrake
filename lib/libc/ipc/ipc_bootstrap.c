//
//  ipc_bootstrap.c
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

#include "../string.h"
#include "../sys/kern_return.h"
#include "ipc_message.h"
#include "ipc_port.h"

ipc_return_t ipc_bootstrap_register(ipc_port_t port, const char *name)
{
	struct
	{
		ipc_header_t header;
		char buffer[255];
	} message;

	size_t length = strlen(name);
	if(length > 255)
		length = 255;

	message.header.reply = port;
	message.header.port = ipc_get_special_port(IPC_SPECIAL_PORT_BOOTSTRAP);
	message.header.flags = IPC_HEADER_FLAG_RESPONSE | IPC_HEADER_FLAG_RESPONSE_BITS(IPC_MESSAGE_RIGHT_COPY_SEND);
	message.header.size = length;
	message.header.id = 0;

	memcpy(message.buffer, name, length);

	return ipc_write(&message.header);
}

ipc_return_t ipc_bootstrap_unregister(const char *name)
{
	struct
	{
		ipc_header_t header;
		char buffer[255];
	} message;

	size_t length = strlen(name);
	if(length > 255)
		length = 255;

	message.header.port = ipc_get_special_port(IPC_SPECIAL_PORT_BOOTSTRAP);
	message.header.size = length;
	message.header.id = 2;

	memcpy(message.buffer, name, length);

	return ipc_write(&message.header);
}

ipc_return_t ipc_bootstrap_lookup(ipc_port_t *port, const char *name)
{
	ipc_port_t replyPort;

	ipc_return_t result = ipc_allocate_port(&replyPort);
	if(result != KERN_SUCCESS)
		return result;

	struct
	{
		ipc_header_t header;
		union
		{
			char buffer[255];
			ipc_return_t response;
		} data;
	} message;

	size_t length = strlen(name);
	if(length > 255)
		length = 255;

	message.header.reply = replyPort;
	message.header.port = ipc_get_special_port(IPC_SPECIAL_PORT_BOOTSTRAP);
	message.header.flags = IPC_HEADER_FLAG_RESPONSE | IPC_HEADER_FLAG_RESPONSE_BITS(IPC_MESSAGE_RIGHT_COPY_SEND_ONCE);
	message.header.size = length;
	message.header.id = 1;

	memcpy(message.data.buffer, name, length);

	result = ipc_write(&message.header);

	if(result != KERN_SUCCESS)
	{
		ipc_deallocate_port(replyPort);
		return result;
	}

	// Retrieve the response
	message.header.port = replyPort;
	message.header.flags = IPC_HEADER_FLAG_BLOCK;
	message.header.size = sizeof(ipc_port_t);

	result = ipc_read(&message.header);
	ipc_deallocate_port(replyPort);

	if(result != KERN_SUCCESS)
		return result;

	if(message.data.response != KERN_SUCCESS)
		return message.data.response;

	*port = message.header.port;

	return KERN_SUCCESS;
}
