//
//  console.h
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

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <prefix.h>
#include <kern/kern_return.h>
#include <libio/core/IOObject.h>
#include <libio/hid/IOHIDKeyboardUtilities.h>
#include <libio/video/IODisplay.h>

namespace OS
{
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

	class Console : public IO::Object
	{
	public:
		// Interface used by the actual driver to request rendering changes
		virtual void ScrollLine() = 0;
		virtual void Putc(char character) = 0;

		virtual void SetColor(Color foreground, Color background) = 0;
		virtual void SetColorIntensity(bool foregroundIntense, bool backgroundIntense) = 0;
		virtual void SetCursor(size_t x, size_t y) = 0;
		virtual void SetCursorHidden(bool hidden) = 0;
		virtual void SetColorPair(Color color, bool intense, bool foreground) = 0;

		size_t GetHeight() const { return height; }
		size_t GetWidth() const { return width; }

		size_t GetCursorX() const { return cursorX; }
		size_t GetCursorY() const { return cursorY; }

	protected:
		Console *InitWithSize(size_t x, size_t y);

		size_t cursorX;
		size_t cursorY;
		size_t width;
		size_t height;

	private:
		IODeclareMetaVirtual(Console)
	};

	void ConsoleInput(IO::KeyboardEvent *event);
	void ConsoleTakeDisplay(IO::Display *display);

	KernReturn<void> ConsoleInit();
}

#endif /* _CONSOLE_H_ */
