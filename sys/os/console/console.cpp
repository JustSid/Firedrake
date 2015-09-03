//
//  pty.h
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

#include <os/pty.h>
#include <libcpp/ringbuffer.h>
#include <libcpp/new.h>
#include <libc/stdlib.h>
#include <libc/stdint.h>
#include <libc/string.h>
#include <kern/kprintf.h>
#include "console.h"

#define kConsoleBufferSize 80*80

namespace OS
{
	static PTY *_consolePty;
	static int _consolePtyFd;

	using ConsoleRingbuffer = std::ringbuffer<char, kConsoleBufferSize>;

	static ConsoleRingbuffer *_consoleTempBuffer;
	static uint8_t *_consoleTempBufferBuffer[sizeof(ConsoleRingbuffer)];

	static size_t _consoleCursorX = 0;
	static size_t _consoleCursorY = 0;
	static size_t _consoleWidth = 80;
	static size_t _consoleHeight = 25;

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

	static uint8_t *_videoAddress = reinterpret_cast<uint8_t *>(0xb8000);

	// Normal, Bright
	static uint8_t ColorTable[2][8] = { { 0x0, 0x4, 0x2, 0x6, 0x1, 0x5, 0x3, 0x7 }, { 0x8, 0xc, 0xa, 0xe, 0x9, 0xd, 0xb, 0xf } };
	static Color _consoleColor[2] = { ColorWhite, ColorBlack }; // 0 is foreground, 1 is background
	static int8_t _colorIntensity[2] = { 0, 0 }; // Same as above

	static State _consoleState;
	static char _consoleAnsiBuffer[128];
	static size_t _consoleAnsiBufferLength = 0;


	void SetColor(size_t x, size_t y, Color foreground, Color background)
	{
		size_t index = (y * _consoleWidth + x) * 2;

		uint8_t foregroundColor = ColorTable[_colorIntensity[0]][foreground];
		uint8_t backgroundColor = ColorTable[_colorIntensity[1]][background];

		_videoAddress[index + 1] = (backgroundColor << 4) | (foregroundColor & 0x0F);
	}

	void ScrollLine()
	{
		for(size_t i = 0; i < _consoleHeight - 1; i ++)
		{
			memcpy(&_videoAddress[(i * _consoleWidth) * 2], &_videoAddress[((i + 1) * _consoleWidth) * 2], _consoleWidth * 2);
		}

		memset(&_videoAddress[((_consoleHeight - 1) * _consoleWidth) * 2], 0, _consoleWidth * 2);
		_consoleCursorY --;
	}

	void ReadColor(uint8_t color, Color &outColor, int8_t &outIntensity)
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
		size_t index = (_consoleCursorY * _consoleWidth + _consoleCursorX) * 2;

		if(_videoAddress[index + 1] != 0)
		{
			ReadColor((_videoAddress[index + 1] >> 4) & 0xf, _consoleColor[0], _colorIntensity[0]);
			ReadColor(_videoAddress[index + 1] & 0xf, _consoleColor[1], _colorIntensity[1]);
		}
	}

	void ParseControlSequence(const char *sequence, char terminator)
	{
		switch(terminator)
		{
			case 'A':
			{
				int num = atoi(sequence);
				while((num --) && _consoleCursorY > 0)
					_consoleCursorY --;

				UpdateColor();
				break;
			}
			case 'B':
			{
				_consoleCursorY += atoi(sequence);
				_consoleCursorY = std::min(_consoleCursorY, _consoleHeight - 1);

				UpdateColor();
				break;
			}

			case 'C':
			{
				_consoleCursorX += atoi(sequence);
				_consoleCursorX = std::min(_consoleCursorX, _consoleWidth - 1);

				UpdateColor();
				break;
			}
			case 'D':
			{
				int num = atoi(sequence);
				while((num --) && _consoleCursorX > 0)
					_consoleCursorX --;

				UpdateColor();
				break;
			}

			case 'E':
			{
				_consoleCursorY += atoi(sequence);
				_consoleCursorY = std::min(_consoleCursorY, _consoleHeight - 1);

				_consoleCursorX = 0;
				UpdateColor();
				break;
			}
			case 'F':
			{
				int num = atoi(sequence);
				while((num --) && _consoleCursorY > 0)
					_consoleCursorY --;

				_consoleCursorX = 0;
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

				_consoleCursorX = std::max(0, std::min(static_cast<int>(_consoleWidth), x));
				_consoleCursorY = std::max(0, std::min(static_cast<int>(_consoleHeight), y));

				UpdateColor();
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
							_consoleColor[1] = ColorBlack;
							_consoleColor[0] = ColorWhite;

							_colorIntensity[0] = 0;
							_colorIntensity[1] = 0;
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
							_consoleColor[0] = static_cast<Color>(code - 30);
							_colorIntensity[0] = brightColor;
							break;

						case 40:
						case 41:
						case 42:
						case 43:
						case 44:
						case 45:
						case 46:
						case 47:
							_consoleColor[1] = static_cast<Color>(code - 40);
							_colorIntensity[1] = brightColor;
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





	void ConsolePuts(const char *string, size_t length)
	{
		if(__expect_false(!_consolePty))
		{
			for(size_t i = 0; i < length; i ++)
				_consoleTempBuffer->push(*string ++);

			return;
		}

		VFS::Write(VFS::Context::GetKernelContext(), _consolePtyFd, string, length);
	}

	void ConsolePTYMasterPutc(char character)
	{
		if(_consoleState == State::Normal)
		{
			switch(character)
			{
				case '\n':
				{
					_consoleCursorX = 0;

					if((++ _consoleCursorY) >= _consoleHeight)
						ScrollLine();

					break;
				}
				case '\t':
				{
					_consoleCursorX += 4;
					break;
				}
				case '\b':
				{
					if(_consoleCursorX > 0)
						_consoleCursorX --;

					_videoAddress[(_consoleCursorY * _consoleWidth + _consoleCursorX) * 2] = 0;
					break;
				}
				case '\033':
				{
					_consoleState = State::Got033;
					break;
				}
				default:
				{
					_videoAddress[(_consoleCursorY * _consoleWidth + _consoleCursorX) * 2] = character;

					SetColor(_consoleCursorX, _consoleCursorY, _consoleColor[0], _consoleColor[1]);

					if((++ _consoleCursorX) >= _consoleWidth)
					{
						_consoleCursorX = 0;

						if((++ _consoleCursorY) >= _consoleHeight)
							ScrollLine();
					}
					break;
				}
			}
		}
		else
		{
			switch(_consoleState)
			{
				case State::Got033:
					_consoleState = State::Got033E; // Just assume the next character was [
					break;

				case State::Got033E:
				{
					if(character >= 64 && character <= 126)
					{
						_consoleAnsiBuffer[_consoleAnsiBufferLength ++] = '\0';
						ParseControlSequence(_consoleAnsiBuffer, character);

						_consoleAnsiBufferLength = 0;
						_consoleState = State::Normal;

						break;
					}

					_consoleAnsiBuffer[_consoleAnsiBufferLength ++] = character;
					break;
				}

				default:
					break;
			}
		}

	}


	void ConsoleInput(IO::KeyboardEvent *event)
	{
		if(event->IsKeyDown())
		{
			char string[2];
			string[0] = event->GetCharacter();
			string[1] = 0;

			if(string[0] == 0)
				return;

			kputs(string); // Use kputs() instead of writing to the PTY directly

		}

		/*if(event->IsKeyDown())
		{
			kprintf("%c", event->GetCharacter());

			switch(event->GetKeyCode())
			{
				case IO::KeyCodeArrowUp:
					kprintf("\033[1A");
					break;
				case IO::KeyCodeArrowDown:
					kprintf("\033[1B");
					break;
				case IO::KeyCodeArrowLeft:
					kprintf("\033[1D");
					break;
				case IO::KeyCodeArrowRight:
					kprintf("\033[1C");
					break;

				default:
					break;
			}
		}*/
	}


	void ConsoleCreatePTY()
	{
		Sys::VM::Directory *directory = Sys::VM::Directory::GetKernelDirectory();
		directory->MapPageRange(0xb8000, 0xb8000, 1, kVMFlagsKernel);

		_consolePty = PTY::Alloc()->Init();
		_consolePty->GetMasterDevice()->SetPutcProc(ConsolePTYMasterPutc);
		_consolePtyFd = VFS::Open(VFS::Context::GetKernelContext(), "/dev/pty001", O_RDWR);

		// Disable \n to \r\n translation
		termios term;
		VFS::Ioctl(VFS::Context::GetKernelContext(), _consolePtyFd, TCGETS, &term);
		term.c_oflag &= ~ONLCR;
		VFS::Ioctl(VFS::Context::GetKernelContext(), _consolePtyFd, TCSETS, &term);

		// Dump the queued up console buffer into the PTY
		char buffer[kConsoleBufferSize];
		size_t i = 0;

		while(_consoleTempBuffer->size())
			buffer[i ++] = _consoleTempBuffer->pop();

		VFS::Write(VFS::Context::GetKernelContext(), _consolePtyFd, buffer, i);
	}

	KernReturn<void> ConsoleInit()
	{
		_consoleTempBuffer = new(_consoleTempBufferBuffer) ConsoleRingbuffer();
		Sys::AddOutputHandler(&ConsolePuts);

		return ErrorNone;
	}
}
