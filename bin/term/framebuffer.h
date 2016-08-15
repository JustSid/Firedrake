//
//  framebuffer.h
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

#ifndef _FRAMEBUFFER_H_
#define _FRAMEBUFFER_H_

#include <sys/mman.h>
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

#endif /* _FRAMEBUFFER_H_ */
