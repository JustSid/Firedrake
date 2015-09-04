//
//  console.cpp
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
#include "console.h"
#include "textconsole.h"
#include "graphicsconsole.h"

#define kConsoleBufferSize 80*80

namespace OS
{
	IODefineMetaVirtual(Console, IO::Object)

	Console *Console::InitWithSize(size_t x, size_t y)
	{
		if(!IO::Object::Init())
			return nullptr;

		width = x;
		height = y;

		cursorX = 0;
		cursorY = 0;

		return this;
	}


	static Mutex _consoleMutex;
	static Console *_console;
	static PTY *_consolePty;
	static int _consolePtyFd;

	using ConsoleRingbuffer = std::ringbuffer<char, kConsoleBufferSize>;

	static ConsoleRingbuffer *_consoleTempBuffer;
	static uint8_t *_consoleTempBufferBuffer[sizeof(ConsoleRingbuffer)];

	static ConsoleRingbuffer *_consoleReplayBuffer;
	static uint8_t *_consoleReplayBufferBuffer[sizeof(ConsoleRingbuffer)];

	enum class State
	{
		Normal,
		Got033,
		Got033E // \033[
	};

	static State _consoleState;
	static char _consoleAnsiBuffer[128];
	static size_t _consoleAnsiBufferLength = 0;


	void ParseControlSequence(const char *sequence, char terminator)
	{
		switch(terminator)
		{
			case 'A':
			{
				int num = atoi(sequence);
				size_t cursorY = _console->GetCursorY();

				while((num --) && cursorY > 0)
					cursorY --;

				_console->SetCursor(_console->GetCursorX(), cursorY);
				break;
			}
			case 'B':
			{
				size_t cursorY = _console->GetCursorY();

				cursorY += atoi(sequence);
				cursorY = std::min(cursorY, _console->GetHeight() - 1);

				_console->SetCursor(_console->GetCursorX(), cursorY);
				break;
			}

			case 'C':
			{
				size_t cursorX = _console->GetCursorX();

				cursorX += atoi(sequence);
				cursorX = std::min(cursorX, _console->GetWidth() - 1);

				_console->SetCursor(cursorX, _console->GetCursorY());
				break;
			}
			case 'D':
			{
				int num = atoi(sequence);
				size_t cursorX = _console->GetCursorX();

				while((num --) && cursorX > 0)
					cursorX --;

				_console->SetCursor(cursorX, _console->GetCursorY());
				break;
			}

			case 'E':
			{
				size_t cursorY = _console->GetCursorY();

				cursorY += atoi(sequence);
				cursorY = std::min(cursorY, _console->GetHeight() - 1);

				_console->SetCursor(0, cursorY);
				break;
			}
			case 'F':
			{
				int num = atoi(sequence);
				size_t cursorY = _console->GetCursorY();

				while((num --) && cursorY > 0)
					cursorY --;

				_console->SetCursor(0, cursorY);
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

				int cursorX = std::max(0, std::min(static_cast<int>(_console->GetWidth()), x));
				int cursorY = std::max(0, std::min(static_cast<int>(_console->GetHeight()), y));

				_console->SetCursor(cursorX, cursorY);
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
							_console->SetColor(ColorWhite, ColorBlack);
							_console->SetColorIntensity(false, false);
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
							_console->SetColorPair(static_cast<Color>(code - 30), brightColor, true);
							break;

						case 40:
						case 41:
						case 42:
						case 43:
						case 44:
						case 45:
						case 46:
						case 47:
							_console->SetColorPair(static_cast<Color>(code - 40), brightColor, false);
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

	void __ConsolePutc(char character)
	{
		if(_consoleState == State::Normal)
		{
			if(character == '\033')
			{
				_consoleState = State::Got033;
				return;
			}

			_console->Putc(character);
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

	void ConsolePTYMasterPutc(char character)
	{
		_consoleMutex.Lock();
		__ConsolePutc(character);
		_consoleReplayBuffer->push(character);
		_consoleMutex.Unlock();
	}


	// The kprintf() output handler, pipe it directly into the PTY
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


	void ConsoleInput(IO::KeyboardEvent *event)
	{
		if(event->IsKeyDown())
		{
			char string[2];
			string[0] = event->GetCharacter();
			string[1] = 0;

			if(string[0] != 0)
			{
				kputs(string); // Use kputs() instead of writing to the PTY directly
				return;
			}
		}

		// Special character handling
		if(event->IsKeyDown())
		{
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
		}
	}

	void ConsoleTakeDisplay(IO::Display *display)
	{
		// We latch onto the first display that presents itself to us
		// And then just never let go of it...
		if(!_console->IsKindOfClass(GraphicsConsole::GetMetaClass()))
		{
			_consoleMutex.Lock();

			_console->Release();
			_console = GraphicsConsole::Alloc()->InitWithDisplay(display);

			// Update the PTY with the new size
			winsize size;
			size.ws_col = static_cast<uint16_t>(_console->GetWidth());
			size.ws_row = static_cast<uint16_t>(_console->GetHeight());

			VFS::Ioctl(VFS::Context::GetKernelContext(), _consolePtyFd, TIOCSWINSZ, &size);

			// Apply the replay buffer
			while(_consoleReplayBuffer->size())
				__ConsolePutc(_consoleReplayBuffer->pop());

			_consoleMutex.Unlock();
		}
	}

	void ConsoleCreatePTY()
	{
		// Start out with the basic text mode console
		_console = TextConsole::Alloc()->Init();

		// Create the PTY
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
		_consoleReplayBuffer = new(_consoleReplayBufferBuffer) ConsoleRingbuffer();

		Sys::AddOutputHandler(&ConsolePuts);

		return ErrorNone;
	}
}
