//
//  textconsole.cpp
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

#include <machine/memory/memory.h>
#include <machine/port.h>
#include "textconsole.h"

namespace OS
{
	IODefineMeta(TextConsole, Console)

	static uint8_t ColorTable[2][8] = { { 0x0, 0x4, 0x2, 0x6, 0x1, 0x5, 0x3, 0x7 }, { 0x8, 0xc, 0xa, 0xe, 0x9, 0xd, 0xb, 0xf } };

	TextConsole *TextConsole::Init()
	{
		if(!OS::Console::InitWithSize(80, 25))
			return nullptr;

		_videoAddress = reinterpret_cast<uint8_t *>(0xb8000);
		_color[0] = ColorWhite;
		_color[1] = ColorBlack;
		_colorIntensity[0] = 0;
		_colorIntensity[1] = 0;

		Sys::VM::Directory *directory = Sys::VM::Directory::GetKernelDirectory();
		directory->MapPageRange(0xb8000, 0xb8000, 1, kVMFlagsKernel);

		return this;
	}

	void TextConsole::ScrollLine()
	{
		for(size_t i = 0; i < height - 1; i ++)
			memcpy(&_videoAddress[(i * width) * 2], &_videoAddress[((i + 1) * width) * 2], width * 2);

		memset(&_videoAddress[((height - 1) * width) * 2], 0, width * 2);
		cursorY --;
	}
	void TextConsole::Putc(char character)
	{
		switch(character)
		{
			case '\n':
			{
				cursorX = 0;

				if((++ cursorY) >= height)
					ScrollLine();

				break;
			}
			case '\t':
			{
				cursorX += 4;
				break;
			}
			case '\b':
			{
				if(cursorX > 0)
					cursorX --;

				_videoAddress[(cursorY * width + cursorX) * 2] = 0;
				break;
			}

			default:
			{
				_videoAddress[(cursorY * width + cursorX) * 2] = character;
				ApplyColor(cursorX, cursorY, _color[0], _color[1]);

				if((++ cursorX) >= width)
				{
					cursorX = 0;

					if((++ cursorY) >= height)
						ScrollLine();
				}
				break;
			}
		}

		ApplyColor(cursorX, cursorY, _color[0], _color[1]);
		SetCursor(cursorX, cursorY);
	}

	void TextConsole::SetColor(Color foreground, Color background)
	{
		_color[0] = foreground;
		_color[1] = background;

		ApplyColor(cursorX, cursorY, foreground, background);
	}
	void TextConsole::SetColorIntensity(bool foregroundIntense, bool backgroundIntense)
	{
		_colorIntensity[0] = foregroundIntense;
		_colorIntensity[1] = backgroundIntense;

		ApplyColor(cursorX, cursorY, _color[0], _color[1]);
	}

	void TextConsole::SetCursor(size_t x, size_t y)
	{
		size_t index = y * width + x;

		cursorX = x;
		cursorY = y;

		outb(0x3d4, 0x0f);
		outb(0x3d5, static_cast<uint8_t>(index & 0xff));

		outb(0x3d4, 0x0e);
		outb(0x3d5, static_cast<uint8_t>((index >> 8) & 0xff));

		UpdateColor();
	}

	void TextConsole::SetCursorHidden(__unused bool hidden)
	{}

	void TextConsole::SetColorPair(Color color, bool intense, bool foreground)
	{
		foreground ? SetColor(color, _color[1]) : SetColor(_color[0], color);
		foreground ? SetColorIntensity(intense, _colorIntensity[1]) : SetColorIntensity(_colorIntensity[0], intense);
	}



	void TextConsole::ApplyColor(size_t x, size_t y, Color foreground, Color background)
	{
		size_t index = (y * width + x) * 2;

		uint8_t foregroundColor = ColorTable[_colorIntensity[0]][foreground];
		uint8_t backgroundColor = ColorTable[_colorIntensity[1]][background];

		_videoAddress[index + 1] = (backgroundColor << 4) | (foregroundColor & 0x0F);
	}

	void TextConsole::UpdateColor()
	{
		size_t index = (cursorY * width + cursorX) * 2;

		if(_videoAddress[index + 1] != 0)
		{
			ReadColor((_videoAddress[index + 1] >> 4) & 0xf, _color[1], _colorIntensity[1]);
			ReadColor(_videoAddress[index + 1] & 0xf, _color[0], _colorIntensity[0]);
		}
	}

	void TextConsole::ReadColor(uint8_t color, Color &outColor, int8_t &outIntensity)
	{
		switch(color)
		{
			case 0x0:
			case 0x8:
				outColor = ColorBlack;
				outIntensity = (color >= 0x8);
				break;
			case 0x4:
			case 0xc:
				outColor = ColorRed;
				outIntensity = (color >= 0x8);
				break;
			case 0x2:
			case 0xa:
				outColor = ColorGreen;
				outIntensity = (color >= 0x8);
				break;
			case 0x6:
			case 0xe:
				outColor = ColorYellow;
				outIntensity = (color >= 0x8);
				break;
			case 0x1:
			case 0x9:
				outColor = ColorBlue;
				outIntensity = (color >= 0x8);
				break;
			case 0x5:
			case 0xd:
				outColor = ColorMagenta;
				outIntensity = (color >= 0x8);
				break;
			case 0x3:
			case 0xb:
				outColor = ColorCyan;
				outIntensity = (color >= 0x8);
				break;
			case 0x7:
			case 0xf:
				outColor = ColorWhite;
				outIntensity = (color >= 0x8);
				break;
		}
	}
}
