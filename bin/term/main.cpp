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
#include <drake/input.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

extern uint8_t *GetFont();

constexpr static uint32_t ColorTable[2][8] = {
	{ 0xff000000, 0xffa00000, 0xff0aaC00, 0xffaaa700, 0xfa0001aa, 0xffa001aa, 0xff0aaaaa, 0xffaaaaaa },
	{ 0xff000000, 0xfff00000, 0xff0ffC00, 0xfffffc00, 0xff0003ff, 0xfff003ff, 0xff0fffff, 0xffffffff }
};

struct Framebuffer
{
	Framebuffer(void *buffer, size_t width, size_t height) :
		_memory(buffer),
		_width(width),
		_height(height)
	{}

	inline void setPixel(size_t x, size_t y, uint32_t color)
	{
		uint32_t *buffer = reinterpret_cast<uint32_t *>(_memory);
		buffer[(y * _width) + x] = color;
	}

	inline void blitCharacter(char character, size_t x, size_t y)
	{
		uint8_t *bitmap = GetFont() + (static_cast<uint8_t>(character) * 16);

		for(int i = 0; i < 16; i ++)
		{
			uint8_t entry = bitmap[i];

			for(int j = 0; j < 8; j ++)
			{
				if((entry & (1 << j)))
					setPixel(x + j, y + i, ColorTable[0][7]);
				else
					setPixel(x + j, y + i, ColorTable[0][0]);
			}
		}
	}

	inline void flush()
	{
		msync(_memory, _width * _height * 4, 0);
	}


	void *_memory;
	size_t _width;
	size_t _height;
};

void runGraphicsConsole(Framebuffer *framebuffer)
{
	framebuffer->blitCharacter('H', 0, 0);
	framebuffer->blitCharacter('e', 8, 0);
	framebuffer->blitCharacter('l', 16, 0);
	framebuffer->blitCharacter('l', 24, 0);
	framebuffer->blitCharacter('o', 32, 0);

	framebuffer->flush();

	while(1)
	{
		thread_yield();
	}
}


int main(__unused int argc, __unused const char *argv[])
{
	// Hardcoded framebuffer and PTY
	int fbfd = open("/dev/framebuffer0", O_RDWR);
	if(fbfd < 0)
		return EXIT_FAILURE;


	int ptyfd = open("/dev/pty001", O_RDWR);
	if(ptyfd < 0)
		return EXIT_FAILURE;

	// mmap the framebuffer
	void *buffer = mmap(nullptr, 1024 * 768 * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	if(!buffer)
		return EXIT_FAILURE;

	// Clear the screen
	Framebuffer framebuffer(buffer, 1024, 768);
	runGraphicsConsole(&framebuffer);

	return 0;
}