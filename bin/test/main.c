//
//  main.c
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2014 by Sidney Just
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

#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/thread.h>
#include <ipc/ipc_message.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_bootstrap.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void send_file_name(const char *name, ipc_port_t port)
{
	struct
	{
		ipc_header_t header;
		char buffer[255];
	} message;

	message.header.port = port;
	message.header.flags = 0;
	message.header.id = 215;
	message.header.size = strlen(name) + 1;

	memcpy(message.buffer, name, message.header.size + 1);
	ipc_write(&message.header);
}

int main(__unused int argc, __unused char *argv[])
{
	puts("Waiting for IPC port\n");

	ipc_port_t port;

	while(ipc_bootstrap_lookup(&port, "com.test.server") != KERN_SUCCESS)
	{}

	printf("Got IPC port (%d)\n", port);

	send_file_name("/etc/about", port);
	send_file_name("/etc/license.txt", port);

	return EXIT_SUCCESS;
}
