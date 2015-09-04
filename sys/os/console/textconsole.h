//
//  textconsole.h
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

#ifndef _TEXTCONSOLE_H_
#define _TEXTCONSOLE_H_

#include "console.h"

namespace OS
{
	class TextConsole : public Console
	{
	public:
		TextConsole *Init();

		void ScrollLine() override;
		void Putc(char character) override;

		void SetColor(Color foreground, Color background) override;
		void SetColorIntensity(bool foregroundIntense, bool backgroundIntense) override;
		void SetCursor(size_t x, size_t y) override;
		void SetCursorHidden(bool hidden) override;
		void SetColorPair(Color color, bool intense, bool foreground) override;

	private:
		void ApplyColor(size_t x, size_t y, Color foreground, Color background);
		void UpdateColor();
		void ReadColor(uint8_t color, Color &outColor, int8_t &outIntensity);

		uint8_t *_videoAddress;
		Color _color[2] = { ColorWhite, ColorBlack }; // 0 is foreground, 1 is background
		int8_t _colorIntensity[2] = { 0, 0 }; // Same as above

		IODeclareMeta(TextConsole)
	};
}

#endif /* _TEXTCONSOLE_H_ */
