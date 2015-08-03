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
#include <machine/port.h>
#include <machine/memory/memory.h>
#include <kern/kprintf.h>
#include "video.h"

namespace Sys
{
	namespace Video
	{
		enum Color
		{
			ColorBlack = 0x0,
			ColorBlue = 0x1,
			ColorGreen = 0x2,
			ColorCyan = 0x3,
			ColorRed = 0x4,
			ColorMagenta = 0x5,
			ColorBrown = 0x6,
			ColorLightGray = 0x7,
			ColorDarkGray = 0x8,
			ColorLightBlue = 0x9,
			ColorLightGreen = 0xA,
			ColorLightCyan = 0xB,
			ColorLightRed = 0xC,
			ColorLightMagenta = 0xD,
			ColorYellow = 0xE,
			ColorWhite = 0xF
		};

		static uint8_t *_videoAddress = reinterpret_cast<uint8_t *>(0xb8000);
		static size_t _videoCursorX = 0;
		static size_t _videoCursorY = 0;

		static size_t _videoWidth = 80;
		static size_t _videoHeight = 25;

		static Color _videoForegroundColor = ColorLightGray;
		static Color _videoBackgroundColor = ColorBlack;


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
			_videoAddress[index + 1] = (background << 4) | (foreground & 0x0F);
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

			SetColor(0, 0, _videoForegroundColor, _videoBackgroundColor);
		}

		void Clear()
		{
			memset(_videoAddress, 0 , _videoWidth * _videoHeight * 2);
			SetCursor(0, 0);
		}


		void Print(const char *string, size_t length)
		{
			for(size_t i = 0; i < length; i ++)
			{
				char character = string[i];
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

			SetCursor(_videoCursorX,  _videoCursorY);
		}
	}

	KernReturn<void> InitVideo()
	{
		VM::Directory *directory = VM::Directory::GetKernelDirectory();
		directory->MapPageRange(0xb8000, 0xb8000, 1, kVMFlagsKernel);

		AddOutputHandler(&Video::Print);

		return ErrorNone;
	}
}
