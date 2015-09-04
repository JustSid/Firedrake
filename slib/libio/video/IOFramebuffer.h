//
//  IOFramebuffer.h
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

#ifndef _IOFRAMEBUFFER_H_
#define _IOFRAMEBUFFER_H_

#include "../core/IOObject.h"

namespace IO
{
	// Colors are represented as 32 bit ARGB colors, 8 bit per component
	class Framebuffer : public Object
	{
	public:
		Framebuffer *InitWithMemory(void *memory, size_t width, size_t height, uint8_t depth);
		Framebuffer *InitWithSize(size_t width, size_t height, uint8_t depth);

		void Dealloc();

		void Clear(uint32_t color);

		void SetPixel(size_t x, size_t y, uint32_t color);
		void FillRect(size_t x, size_t y, size_t width, size_t height, uint32_t color);

		size_t GetWidth() const { return _width; }
		size_t GetHeight() const { return _height; }

	private:
		bool _ownsMemory;
		void *_memory;
		size_t _width;
		size_t _height;
		uint8_t _depth;

		IODeclareMeta(Framebuffer)
	};
}

#endif /* _IOFRAMEBUFFER_H_ */
