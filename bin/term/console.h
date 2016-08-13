//
//  console.h
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

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <stdint.h>
#include "framebuffer.h"

class Console
{
public:
	enum Color
	{
		ColorBlack,
		ColorRed,
		ColorGreen,
		ColorYellow,
		ColorBlue,
		ColorMagenta,
		ColorCyan,
		ColorWhite
	};

	enum class State
	{
		Normal,
		Got033,
		Got033E // \033[
	};

	Console(Framebuffer *framebuffer);

	void SetCursor(size_t x, size_t y);
	void ScrollLine();

	void Putc(char character);

	size_t GetCursorX() const { return _cursorX; }
	size_t GetCursorY() const { return _cursorY; }
	size_t GetWidth() const { return _width; }
	size_t GetHeight()const { return _height; }

private:
	void ParseControlSequence(char terminator);

	State _state;
	Framebuffer *_framebuffer;

	size_t _width;
	size_t _height;

	size_t _cursorX;
	size_t _cursorY;

	__unused Color _color[2]; // 0 is foreground, 1 is background
	__unused int8_t _colorIntensity[2]; // Same as above

	char _ansiBuffer[128];
	size_t _ansiBufferLength;
};

#endif /* _CONSOLE_H_ */
