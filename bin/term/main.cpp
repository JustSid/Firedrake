//
//  main.cpp
//  term
//
//  Created by Sidney Just
//  Copyright (c) 2016 by Sidney Just
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

// Term provides the master endpoint for a pseudo-terminal in Firedrake
// as well as managing the on-screen representation.

#include <sys/mman.h>
#include <sys/unistd.h>
#include <sys/errno.h>
#include <sys/thread.h>
#include <sys/fcntl.h>
#include <sys/termios.h>
#include <sys/ioctl.h>
#include <drake/input.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "framebuffer.h"
#include "console.h"


void runGraphicsConsole(Framebuffer *framebuffer, int fd)
{
	Console console(framebuffer);

	Keyboard keyboard;
	keyboard_open(&keyboard);

	while(1)
	{
		char buffer[128];
		size_t offset = 0;

		// Keyboard input
		while(1)
		{
			KeyboardBuffer input;
			if(!keyboard_read(&keyboard, &input))
				break;

			if(input.isDown && input.character)
				buffer[offset ++] = input.character;
		}

		if(offset > 0)
			write(fd, buffer, offset);

		//
		size_t bytesIn = read(fd, buffer, 128);

		if(bytesIn > 0)
		{
			for(size_t i = 0; i < bytesIn; i ++)
				console.Putc(buffer[i]);

			framebuffer->flush();
		}

		thread_yield();
	}
}


int main(__unused int argc, __unused const char *argv[])
{
	// Hardcoded framebuffer and PTY
	int fbfd = open("/dev/framebuffer0", O_RDWR);
	if(fbfd < 0)
		return EXIT_FAILURE;

	// mmap the framebuffer
	void *buffer = mmap(nullptr, 1024 * 768 * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	if(!buffer)
		return EXIT_FAILURE;


	// Open up the PTY
	int ptyfd = open("/dev/pty001", O_RDWR);
	if(ptyfd < 0)
		return EXIT_FAILURE;

	termios term;
	ioctl(ptyfd, TCGETS, &term);
	term.c_oflag &= ~ONLCR;
	ioctl(ptyfd, TCSETS, &term);

	int fd = open("/tmp/_term", O_CREAT);

	// Create the console
	Framebuffer framebuffer(buffer, 1024, 768);
	runGraphicsConsole(&framebuffer, ptyfd);

	close(fd);

	return 0;
}