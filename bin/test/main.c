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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void puts(const char *string)
{
	char buffer[255];

	ipc_header_t *header = (ipc_header_t *)buffer;
	header->port = ipc_get_special_port(IPC_SPECIAL_PORT_PUT);
	header->flags = 0;
	header->size = strlen(string) + 1;

	char *data = (char *)IPC_GET_DATA(header);
	strcpy(data, string);

	ipc_write(header);
}

void printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	char buffer[512];
	vsnprintf(buffer, 512, format, args);

	puts(buffer);
	va_end(args);
}


void test(void *arg)
{
	int fd = open((char *)arg, O_RDONLY);
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

	puts("\n");
	close(fd);
}

int main(int argc, char *argv[])
{
	tid_t thread = thread_create(&test, "/etc/about");
	thread_join(thread);

	return EXIT_SUCCESS;
}
