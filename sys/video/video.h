//
//  video.h
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2013 by Sidney Just
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

#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <libc/stdint.h>
#include <libc/stddef.h>

class VideoDevice
{
public:
	enum class Color : char
	{
		Black = 0x0,
		White = 0xf,
		Blue = 0x1,
		Green,
		Cyan,
		Red,
		Magenta,
		Brown,
		LightGray,
		DarkGray,
		LightBlue,
		LightGreen,
		LightCyan,
		LightRed,
		LightMagenta,
		Yellow
	};

	VideoDevice();
	virtual ~VideoDevice();

	virtual bool IsText() = 0;

	virtual size_t GetWidth()  = 0;
	virtual size_t GetHeight() = 0;
	virtual size_t GetDepth() = 0;

	Color GetBackgroundColor() { return backgroundColor; }
	Color GetForegroundColor() { return foregroundColor; }
	size_t GetCursorX() { return cursorX; }
	size_t GetCursorY() { return cursorY; }

	virtual void SetColors(Color foreground, Color background) = 0;
	virtual void SetCursor(size_t x, size_t y) = 0;

	virtual void WriteString(const char *string) = 0;
	virtual void ScrollLines(size_t numLines) = 0;
	virtual void Clear() = 0;

protected:
	Color backgroundColor;
	Color foregroundColor;

	size_t cursorX;
	size_t cursorY;
};

bool vd_init();

VideoDevice *vd_getActiveDevice();

#endif /* _VIDEO_H_ */
