//
//  textdevice.h
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

#include "video.h"

#ifndef _TEXTDEVICE_H_
#define _TEXTDEVICE_H_

class TextVideoDevice : public VideoDevice
{
public:
	TextVideoDevice();

	bool IsText() { return true; }

	size_t GetWidth() override { return _width; }
	size_t GetHeight() override { return _height; }
	size_t GetDepth() override { return 0; }

	void SetColors(Color foreground, Color background);
	void SetCursor(size_t x, size_t y);

	void WriteString(const char *string);
	void ScrollLines(size_t lines);
	void Clear();

private:
	void Putc(char character);
	void ColorPoint(size_t x, size_t y, Color foreground, Color background);

	uint8_t *_base;
	size_t _width;
	size_t _height;
};

#endif /* _TEXTDEVICE_H_ */
