//
//  graphicsconsole.cpp
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

#include "graphicsconsole.h"

extern uint8_t *GetFont();

namespace OS
{
	IODefineMeta(GraphicsConsole, Console)

	static uint32_t ColorTable[2][8] = {
		{ 0xff000000, 0xffa00000, 0xff0aaC00, 0xffaaa700, 0xfa0001aa, 0xffa001aa, 0xff0aaaaa, 0xffaaaaaa },
		{ 0xff000000, 0xfff00000, 0xff0ffC00, 0xfffffc00, 0xff0003ff, 0xfff003ff, 0xff0fffff, 0xffffffff }
	};

	GraphicsConsole *GraphicsConsole::InitWithDisplay(IO::Display *display)
	{
		IO::Framebuffer *framebuffer = display->GetFramebuffer();

		size_t width = framebuffer->GetWidth() / 8;
		size_t height = framebuffer->GetHeight() / 16;

		if(!Console::InitWithSize(width, height))
			return nullptr;

		_display = display->Retain();
		_framebuffer = framebuffer;
		_characters = new Character[width * height];

		return this;
	}

	void GraphicsConsole::ScrollLine()
	{}

	void GraphicsConsole::Putc(char character)
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

				BlitCharacter('\0');
				break;
			}

			default:
			{
				BlitCharacter(character);

				if((++ cursorX) >= width)
				{
					cursorX = 0;

					if((++ cursorY) >= height)
						ScrollLine();
				}
				break;
			}
		}
	}

	void GraphicsConsole::BlitCharacter(char character)
	{
		uint8_t *bitmap = GetFont() + (static_cast<uint8_t>(character) * 16);

		size_t x = cursorX * 8;
		size_t y = cursorY * 16;

		for(int i = 0; i < 16; i ++)
		{
			uint8_t entry = bitmap[i];

			for(int j = 0; j < 8; j ++)
			{
				if((entry & (1 << j)))
					_framebuffer->SetPixel(x + j, y + i, ColorTable[_colorIntensity[0]][_color[0]]);
				else
					_framebuffer->SetPixel(x + j, y + i, ColorTable[_colorIntensity[0]][_color[1]]);
			}
		}

		Character *c = _characters + (cursorY * width + cursorX);
		c->color[0] = _color[0];
		c->color[1] = _color[1];
		c->intensity[0] = _colorIntensity[0];
		c->intensity[1] = _colorIntensity[1];
	}

	void GraphicsConsole::SetColor(Color foreground, Color background)
	{
		_color[0] = foreground;
		_color[1] = background;

		Character *character = _characters + (cursorY * width + cursorX);

		character->color[0] = foreground;
		character->color[1] = background;
	}
	void GraphicsConsole::SetColorIntensity(bool foregroundIntense, bool backgroundIntense)
	{
		_colorIntensity[0] = foregroundIntense;
		_colorIntensity[1] = backgroundIntense;

		Character *character = _characters + (cursorY * width + cursorX);
		character->intensity[0] = foregroundIntense;
		character->intensity[1] = backgroundIntense;
	}
	void GraphicsConsole::SetCursor(size_t x, size_t y)
	{
		cursorX = x;
		cursorY = y;
	}
	void GraphicsConsole::SetCursorHidden(bool hidden)
	{}
	void GraphicsConsole::SetColorPair(Color color, bool intense, bool foreground)
	{
		Character *character = _characters + (cursorY * width + cursorX);

		foreground ? SetColor(color, character->color[1]) : SetColor(character->color[0], color);
		foreground ? SetColorIntensity(intense, character->intensity[1]) : SetColorIntensity(character->intensity[0], intense);
	}
}
