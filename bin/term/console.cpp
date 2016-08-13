//
//  console.cpp
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

#include <algorithm.h>
#include <stdlib.h>
#include <string.h>

#include "console.h"

Console::Console(Framebuffer *framebuffer) :
	_state(State::Normal),
	_framebuffer(framebuffer),
	_width(framebuffer->_width / 8),
	_height(framebuffer->_height / 16),
	_cursorX(0),
	_cursorY(0),
	_ansiBufferLength(0)
{
	_color[0] = ColorWhite;
	_color[1] = ColorBlack;

	_colorIntensity[0] = _colorIntensity[1] = 0;
}

void Console::SetCursor(size_t x, size_t y)
{
	_cursorX = x;
	_cursorY = y;
}

void Console::ScrollLine()
{

}

void Console::Putc(char character)
{
	switch(_state)
	{
		case State::Normal:
		{
			switch(character)
			{
				case '\n':
				{
					_cursorX = 0;

					if((++ _cursorY) >= _height)
						ScrollLine();

					break;
				}
				case '\t':
				{
					_cursorX += 4;
					break;
				}
				case '\b':
				{
					if(_cursorX > 0)
						_cursorX --;

					_framebuffer->blitCharacter('\0', _cursorX * 8, _cursorY * 16);
					break;
				}

				default:
				{
					_framebuffer->blitCharacter(character, _cursorX * 8, _cursorY * 16);

					if((++ _cursorX) >= _width)
					{
						_cursorX = 0;

						if((++ _cursorY) >= _height)
							ScrollLine();
					}
					break;
				}
			}

			break;
		}

		case State::Got033:
			_state = State::Got033E; // Just assume the next character was [
			break;

		case State::Got033E:
		{
			if(character >= 64 && character <= 126)
			{
				_ansiBuffer[_ansiBufferLength ++] = '\0';
				ParseControlSequence(character);

				_ansiBufferLength = 0;
				_state = State::Normal;

				break;
			}

			_ansiBuffer[_ansiBufferLength ++] = character;
			break;
		}
	}
}

void Console::ParseControlSequence(char terminator)
{
	const char *sequence = _ansiBuffer;

	switch(terminator)
	{
		case 'A':
		{
			int num = atoi(sequence);
			size_t cursorY = GetCursorY();

			while((num --) && cursorY > 0)
				cursorY --;

			SetCursor(GetCursorX(), cursorY);
			break;
		}
		case 'B':
		{
			size_t cursorY = GetCursorY();

			cursorY += atoi(sequence);
			cursorY = std::min(cursorY, GetHeight() - 1);

			SetCursor(GetCursorX(), cursorY);
			break;
		}

		case 'C':
		{
			size_t cursorX = GetCursorX();

			cursorX += atoi(sequence);
			cursorX = std::min(cursorX, GetWidth() - 1);

			SetCursor(cursorX, GetCursorY());
			break;
		}
		case 'D':
		{
			int num = atoi(sequence);
			size_t cursorX = GetCursorX();

			while((num --) && cursorX > 0)
				cursorX --;

			SetCursor(cursorX, GetCursorY());
			break;
		}

		case 'E':
		{
			size_t cursorY = GetCursorY();

			cursorY += atoi(sequence);
			cursorY = std::min(cursorY, GetHeight() - 1);

			SetCursor(0, cursorY);
			break;
		}
		case 'F':
		{
			int num = atoi(sequence);
			size_t cursorY = GetCursorY();

			while((num --) && cursorY > 0)
				cursorY --;

			SetCursor(0, cursorY);
			break;
		}

		case 'H':
		{

			int x = atoi(sequence);

			do {
				sequence ++;
			} while(isdigit(*sequence));

			while(*sequence && !isdigit(*sequence))
				sequence ++;

			int y = atoi(sequence);

			int cursorX = std::max(0, std::min(static_cast<int>(GetWidth()), x));
			int cursorY = std::max(0, std::min(static_cast<int>(GetHeight()), y));

			SetCursor(cursorX, cursorY);
			break;
		}


		case 'J':
		{
			/*int code = atoi(sequence);

			if(code == 1)
			{
				size_t index = (_videoCursorY * _videoWidth + _videoCursorX) * 2;
				memset(_videoAddress, 0, index);
			}
			else if(code == 2)
			{
				memset(_videoAddress, 0 , _videoWidth * _videoHeight * 2);
			}
			else
			{
				size_t index = (_videoCursorY * _videoWidth + _videoCursorX) * 2;
				size_t size = (_videoWidth * _videoHeight * 2) - index;

				memset(_videoAddress + index, 0 , size);
			}*/

			break;
		}

		case 'm':
		{
			bool brightColor = false;

			while(*sequence)
			{
				int code = atoi(sequence);

				switch(code)
				{
					case 0:
						//SetColor(ColorWhite, ColorBlack);
						//SetColorIntensity(false, false);
						break;

					case 1:
						brightColor = true;
						break;

					case 30:
					case 31:
					case 32:
					case 33:
					case 34:
					case 35:
					case 36:
					case 37:
						//SetColorPair(static_cast<Color>(code - 30), brightColor, true);
						break;

					case 40:
					case 41:
					case 42:
					case 43:
					case 44:
					case 45:
					case 46:
					case 47:
						//SetColorPair(static_cast<Color>(code - 40), brightColor, false);
						break;
				}

				// Jump over the code and a potential delimiter
				do {
					sequence ++;
				} while(isdigit(*sequence));

				while(*sequence && !isdigit(*sequence))
					sequence ++;
			}

			break;
		}
	}
}
