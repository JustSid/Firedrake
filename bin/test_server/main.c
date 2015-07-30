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
#include <sys/task.h>
#include <ipc/ipc_message.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_bootstrap.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void print_file(const char *arg)
{
	int fd = open(arg, O_RDONLY);
	char temp[129];

	off_t left = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	while(left > 0)
	{
		int toRead = left;
		if(toRead > 128)
			toRead = 128;

		int length = read(fd, temp, toRead);
		temp[length] = '\0';
		puts(temp);

		left -= length;
	}

	puts("\n\n");
	close(fd);
}

int main(int argc, char *argv[])
{
	ipc_port_t port;

	ipc_allocate_port(&port);
	ipc_bootstrap_register(port, "com.test.server");

	task_name("Test Server");

	while(1)
	{
		struct
		{
			ipc_header_t header;
			char buffer[255];
		} message;

		message.header.port = port;
		message.header.flags = IPC_HEADER_FLAG_BLOCK;
		message.header.size = 255;

		ipc_return_t result = ipc_read(&message.header);
		if(result == KERN_SUCCESS)
		{
			char file[255];
			strlcpy(file, message.buffer, 255);

			printf("Printing file: %s\n", file);
			print_file(file);
		}
	}

	return EXIT_SUCCESS;
}
