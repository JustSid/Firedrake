//
//  IOFramebuffer.cpp
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

#include "IOFramebuffer.h"
#include "../core/IORuntime.h"

#if __KERNEL
#include <kern/kalloc.h>
#else
extern "C" void __libkern_registerFramebuffer(IO::Framebuffer *buffer);
extern "C" void __libkern_unregisterFramebuffer(IO::Framebuffer *buffer);
#endif

namespace IO
{
	__IODefineIOCoreMeta(Framebuffer, Object)

	Framebuffer *Framebuffer::InitWithMemory(void *memory, size_t width, size_t height, uint8_t depth)
	{
		if(!Object::Init())
			return nullptr;

		IOAssert(depth == 4, "Only 32bit pixels are supported...");

		_ownsMemory = false;
		_memory = memory;
		_width = width;
		_height = height;
		_depth = depth;

#ifndef __KERNEL
		__libkern_registerFramebuffer(this);
#endif

		return this;
	}
	Framebuffer *Framebuffer::InitWithSize(size_t width, size_t height, uint8_t depth)
	{
		void *memory = kalloc(width * height * depth);
		if(!InitWithMemory(memory, width, height, depth))
		{
			kfree(memory);
			return nullptr;
		}

		_ownsMemory = true;

#ifndef __KERNEL
		__libkern_registerFramebuffer(this);
#endif

		return this;
	}

	void Framebuffer::Dealloc()
	{
#ifndef __KERNEL
		__libkern_unregisterFramebuffer(this);
#endif

		if(_ownsMemory)
			kfree(_memory);

		Object::Dealloc();
	}

	void Framebuffer::Clear(uint32_t color)
	{
		uint32_t *buffer = reinterpret_cast<uint32_t *>(_memory);
		size_t length = _width * _height;

		for(size_t i = 0; i < length; i ++)
		{
			buffer[i] = color;
		}
	}

	void Framebuffer::SetPixel(size_t x, size_t y, uint32_t color)
	{
		uint32_t *buffer = reinterpret_cast<uint32_t *>(_memory);
		buffer[(y * _width) + x] = color;
	}

	void Framebuffer::FillRect(size_t x, size_t y, size_t width, size_t height, uint32_t color)
	{
		uint32_t tmp[50];

		for(size_t i = 0; i < 50; i ++)
			tmp[i] = color;

		uint32_t *buffer = reinterpret_cast<uint32_t *>(_memory);

		for(size_t i = 0; i < height; i ++)
		{
			uint32_t offset = (y * _width) + x;
			size_t left = width;

			while(left > 0)
			{
				size_t copy = (left < 50) ? left : 50;
				memcpy(buffer + offset, tmp, copy * sizeof(uint32_t));

				offset += copy;
				left -= copy;
			}

			y ++;
		}
	}

	void Framebuffer::InvalidateBuffer(__unused size_t offset, __unused size_t length)
	{}
}
