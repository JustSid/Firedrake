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
	char buffer[255 + sizeof(ipc_header_t)];

	size_t length = strlen(name);
	if(length > 255)
		length = 255;

	ipc_header_t *header = (ipc_header_t *)buffer;
	header->reply = port;
	header->port = ipc_get_special_port(IPC_SPECIAL_PORT_BOOTSTRAP);
	header->flags = IPC_HEADER_FLAG_RESPONSE | IPC_HEADER_FLAG_RESPONSE_BITS(IPC_MESSAGE_RIGHT_COPY_SEND);
	header->size = length;
	header->id = 0;

	char *data = (char *)IPC_GET_DATA(header);
	memcpy(data, name, length);

	return ipc_write(header);
}

ipc_return_t ipc_bootstrap_unregister(const char *name)
{
	char buffer[255 + sizeof(ipc_header_t)];

	size_t length = strlen(name);
	if(length > 255)
		length = 255;

	ipc_header_t *header = (ipc_header_t *)buffer;
	header->port = ipc_get_special_port(IPC_SPECIAL_PORT_BOOTSTRAP);
	header->size = length;
	header->id = 2;

	char *data = (char *)IPC_GET_DATA(header);
	memcpy(data, name, length);

	return ipc_write(header);
}

ipc_return_t ipc_bootstrap_lookup(ipc_port_t *port, const char *name)
{
	ipc_port_t replyPort = ipc_thread_port();

	char buffer[255 + sizeof(ipc_header_t)];

	size_t length = strlen(name);
	if(length > 255)
		length = 255;

	ipc_header_t *header = (ipc_header_t *)buffer;
	header->reply = replyPort;
	header->port = ipc_get_special_port(IPC_SPECIAL_PORT_BOOTSTRAP);
	header->flags = IPC_HEADER_FLAG_RESPONSE | IPC_HEADER_FLAG_RESPONSE_BITS(IPC_MESSAGE_RIGHT_COPY_SEND_ONCE);
	header->size = length;
	header->id = 1;

	char *data = (char *)IPC_GET_DATA(header);
	memcpy(data, name, length);

	ipc_return_t result = ipc_write(header);

	if(result != KERN_SUCCESS)
		return result;

	// Retrieve the response
	header->port = replyPort;
	header->flags = IPC_HEADER_FLAG_BLOCK;
	header->size = sizeof(ipc_port_t);

	result = ipc_read(header);

	if(result != KERN_SUCCESS)
		return result;

	ipc_return_t response = *(ipc_return_t *)IPC_GET_DATA(header);
	if(response != KERN_SUCCESS)
		return response;

	*port = header->port;

	return KERN_SUCCESS;
}
