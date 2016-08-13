//
//  main.cpp
//  Mishell
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

#include <sys/unistd.h>
#include <sys/errno.h>
#include <sys/thread.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <string.h>

static int ttyFD;

void WriteToTTY(const char *string)
{
	write(ttyFD, string, strlen(string));
}

void ExecuteCommand(const char *command)
{
	if(strcmp(command, "exit") == 0)
		exit(0);

	WriteToTTY("Unknown command '");
	WriteToTTY(command);
	WriteToTTY("'\n");
}

void ReadCommand()
{
	WriteToTTY("> ");

	while(1)
	{
		char buffer[1024];
		size_t received = read(ttyFD, buffer, 1024);

		if(received > 0)
		{
			buffer[received] = '\0';

			size_t length = strlen(buffer);

			if(length == 0)
				return;

			for(size_t i = length - 1; i > 0; i --)
			{
				if(buffer[i] == '\n' || buffer[i] == '\r')
				{
					buffer[i] = '\0';
					continue;
				}

				break;
			}

			ExecuteCommand(buffer);
			return;
		}

		thread_yield();
	}
}

int main(__unused int argc, __unused const char *argv[])
{
	errno = 0;

	do {

		int fd = open("/tmp/_term", O_RDONLY);
		if(fd < 0)
			continue;

		close(fd);
		break;

	} while(1);

	ttyFD = open("/dev/tty001", O_RDWR);
	if(ttyFD < 0)
		return -1;

	WriteToTTY("Mishell, on /dev/tty001 reporting for duty\n"); // Cheating, but I mean, we know which tty...

	while(1)
	{
		ReadCommand();
		thread_yield();
	}

	return 0;
}
