//
//  pc/video.cpp
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2014 by Sidney Just
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

#include <libc/stdint.h>
#include <libc/string.h>
#include <libc/stdlib.h>
#include <machine/port.h>
#include <machine/memory/memory.h>
#include <libcpp/algorithm.h>
#include <kern/kprintf.h>
#include "video.h"

namespace Sys
{
	namespace Video
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

		uint8_t NormalColorTable[] = { 0x0, 0x4, 0x2, 0x6, 0x1, 0x5, 0x3, 0x7 };
		uint8_t BrightColorTable[] = { 0x8, 0xc, 0xa, 0xe, 0x9, 0xd, 0xb, 0xf };

		static uint8_t *_videoAddress = reinterpret_cast<uint8_t *>(0xb8000);
		static size_t _videoCursorX = 0;
		static size_t _videoCursorY = 0;

		static size_t _videoWidth = 80;
		static size_t _videoHeight = 25;

		static Color _videoForegroundColor = ColorWhite;
		static Color _videoBackgroundColor = ColorBlack;

		static bool _videoForegroundColorIntensity = false;
		static bool _videoBackgroundColorIntensity = false;

		void ScrollLine()
		{
			for(size_t i = 0; i < _videoHeight - 1; i ++)
			{
				memcpy(&_videoAddress[(i * _videoWidth) * 2], &_videoAddress[((i + 1) * _videoWidth) * 2], _videoWidth * 2);
			}

			memset(&_videoAddress[((_videoHeight - 1) * _videoWidth) * 2], 0, _videoWidth * 2);
			_videoCursorY --;
		}

		void SetColor(size_t x, size_t y, Color foreground, Color background)
		{
			size_t index = (y * _videoWidth + x) * 2;

			uint8_t foregroundColor = _videoForegroundColorIntensity ? BrightColorTable[foreground] : NormalColorTable[foreground];
			uint8_t backgroundColor = _videoBackgroundColorIntensity ? BrightColorTable[background] : NormalColorTable[background];

			_videoAddress[index + 1] = (backgroundColor << 4) | (foregroundColor & 0x0F);
		}

		void SetCursor(size_t x, size_t y)
		{
			size_t index = y * _videoWidth + x;

			_videoCursorX = x;
			_videoCursorY = y;

			outb(0x3d4, 0x0f);
			outb(0x3d5, static_cast<uint8_t>(index & 0xff));

			outb(0x3d4, 0x0e);
			outb(0x3d5, static_cast<uint8_t>((index >> 8) & 0xff));

			SetColor(x, y, _videoForegroundColor, _videoBackgroundColor);
		}

		void ReadColor(uint8_t color, Color &outColor, bool &outIntensity)
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

		void UpdateColor()
		{
			size_t index = (_videoCursorY * _videoWidth + _videoCursorX) * 2;

			if(_videoAddress[index + 1] != 0)
			{
				ReadColor((_videoAddress[index + 1] >> 4) & 0xf, _videoBackgroundColor, _videoBackgroundColorIntensity);
				ReadColor(_videoAddress[index + 1] & 0xf, _videoForegroundColor, _videoForegroundColorIntensity);
			}
		}

		void Clear()
		{
			memset(_videoAddress, 0 , _videoWidth * _videoHeight * 2);
			SetCursor(0, 0);
		}

		void ParseControlSequence(const char *sequence, char terminator)
		{
			switch(terminator)
			{
				case 'A':
				{
					int num = atoi(sequence);
					while((num --) && _videoCursorY > 0)
						_videoCursorY --;

					UpdateColor();
					break;
				}
				case 'B':
				{
					_videoCursorY += atoi(sequence);
					_videoCursorY = std::min(_videoCursorY, _videoHeight - 1);

					UpdateColor();
					break;
				}

				case 'C':
				{
					_videoCursorX += atoi(sequence);
					_videoCursorX = std::min(_videoCursorX, _videoWidth - 1);

					UpdateColor();
					break;
				}
				case 'D':
				{
					int num = atoi(sequence);
					while((num --) && _videoCursorX > 0)
						_videoCursorX --;

					UpdateColor();
					break;
				}

				case 'E':
				{
					_videoCursorY += atoi(sequence);
					_videoCursorY = std::min(_videoCursorY, _videoHeight - 1);

					_videoCursorX = 0;
					UpdateColor();
					break;
				}
				case 'F':
				{
					int num = atoi(sequence);
					while((num --) && _videoCursorY > 0)
						_videoCursorY --;

					_videoCursorX = 0;
					UpdateColor();
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

					_videoCursorX = std::max(0, std::min(static_cast<int>(_videoWidth), x));
					_videoCursorY = std::max(0, std::min(static_cast<int>(_videoHeight), y));

					UpdateColor();
					break;
				}


				case 'J':
				{
					int code = atoi(sequence);

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
					}

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
								_videoBackgroundColor = ColorBlack;
								_videoForegroundColor = ColorWhite;
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
								_videoForegroundColor = static_cast<Color>(code - 30);
								_videoForegroundColorIntensity = brightColor;
								break;

							case 40:
							case 41:
							case 42:
							case 43:
							case 44:
							case 45:
							case 46:
							case 47:
								_videoBackgroundColor = static_cast<Color>(code - 40);
								_videoBackgroundColorIntensity = brightColor;
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


		void Print(const char *string, size_t length)
		{
			static char controlSequence[32];
			static size_t controlSequenceLength = 0;
			static bool inControlSequence = false;

			for(size_t i = 0; i < length; i ++)
			{
				char character = string[i];

				if(inControlSequence)
				{
					if(character >= 64 && character <= 126)
					{
						controlSequence[controlSequenceLength ++] = '\0';
						ParseControlSequence(controlSequence, character);

						controlSequenceLength = 0;
						inControlSequence = false;
					}
					else
					{
						controlSequence[controlSequenceLength ++] = character;
					}
				}
				else
				{
					switch(character)
					{
						case '\n':
						{
							_videoCursorX = 0;

							if((++ _videoCursorY) >= _videoHeight)
								ScrollLine();

							break;
						}
						case '\t':
						{
							_videoCursorX += 4;
							break;
						}
						case '\b':
						{
							if(_videoCursorX > 0)
								_videoCursorX --;

							_videoAddress[(_videoCursorY * _videoWidth + _videoCursorX) * 2] = 0;
							break;
						}
						case '\033':
						{
							inControlSequence = true;
							i ++; // Jump over the [
							break;
						}
						default:
						{
							_videoAddress[(_videoCursorY * _videoWidth + _videoCursorX) * 2] = character;
							SetColor(_videoCursorX, _videoCursorY, _videoForegroundColor, _videoBackgroundColor);

							if((++ _videoCursorX) >= _videoWidth)
							{
								_videoCursorX = 0;

								if((++ _videoCursorY) >= _videoHeight)
									ScrollLine();
							}
							break;
						}
					}
				}
			}

			SetCursor(_videoCursorX,  _videoCursorY);
		}
	}

	KernReturn<void> InitVideo()
	{
		VM::Directory *directory = VM::Directory::GetKernelDirectory();
		directory->MapPageRange(0xb8000, 0xb8000, 1, kVMFlagsKernel);

		Video::Clear();
		AddOutputHandler(&Video::Print);

		return ErrorNone;
	}
}
