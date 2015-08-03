//
//  IOHIDPS2KeyboardService.cpp
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

#include <port.h>
#include <libkern.h>
#include <core/IORuntime.h>
#include "IOHIDPS2KeyboardService.h"

/**
 * Only implements the scan code set #2
 * http://www.computer-engineering.org/ps2keyboard/scancodes2.html
 **/

#define kIOPS2ControllerDataPort    0x60
#define kIOPS2ControllerCommandPort 0x64
#define kIOPS2ControllerStatusPort  0x64

#define kIOPS2ControllerStatusOutputBuffer (1 << 0)
#define kIOPS2ControllerStatusInputBuffer  (1 << 1)
#define kIOPS2ControllerStatusSystemFlag   (1 << 2)

namespace IO
{
	IODefineMeta(HIDPS2KeyboardService, HIDKeyboardService)

	static constexpr uint32_t HIDPS2KeyboardTranslationTable[] =
		{
			/* 0x0 */
			0x0,
			KeyCodeF9,
			0x0,
			KeyCodeF5,
			KeyCodeF3,
			KeyCodeF1,
			KeyCodeF2,
			KeyCodeF12,
			0x0,
			KeyCodeF10,
			KeyCodeF8,
			KeyCodeF6,
			KeyCodeF4,
			KeyCodeTab,
			0x0, // Really `
			0x0,

			/* 0x10 */
			0x0,
			KeyCodeLeftOption,
			KeyCodeLeftShift,
			0x0,
			KeyCodeLeftControl,
			KeyCodeQ,
			KeyCode1,
			0x0,
			0x0,
			0x0,
			KeyCodeZ,
			KeyCodeS,
			KeyCodeA,
			KeyCodeW,
			KeyCode2,
			KeyCodeLeftGUI,

			/* 0x20 */
			0x0,
			KeyCodeC,
			KeyCodeX,
			KeyCodeD,
			KeyCodeE,
			KeyCode4,
			KeyCode3,
			KeyCodeRightGUI,
			0x0,
			KeyCodeSpace,
			KeyCodeV,
			KeyCodeF,
			KeyCodeT,
			KeyCodeR,
			KeyCode5,
			0x0,

			/* 0x30 */
			0x0,
			KeyCodeN,
			KeyCodeB,
			KeyCodeH,
			KeyCodeG,
			KeyCodeY,
			KeyCode6,
			0x0,
			0x0,
			0x0,
			KeyCodeM,
			KeyCodeJ,
			KeyCodeU,
			KeyCode7,
			KeyCode8,
			0x0,

			/* 0x40 */
			0x0,
			KeyCodeComma,
			KeyCodeK,
			KeyCodeI,
			KeyCodeO,
			KeyCode0,
			KeyCode9,
			0x0,
			0x0,
			KeyCodeDot,
			KeyCodeSlash,
			KeyCodeL,
			0x0, // Really ;
			KeyCodeP,
			0x0,
			0x0,

			/* 0x50 */
			0x0,
			0x0,
			0x0, // Really '
			0x0,
			0x0, // [
			0x0, // =
			0x0,
			0x0,
			0x0,
			KeyCodeRightShift,
			KeyCodeEnter,
			0x0, // ]
			0x0,
			0x0,
			0x0
		};

	HIDPS2KeyboardService *HIDPS2KeyboardService::Init()
	{
		if(!HIDKeyboardService::Init())
			return nullptr;

		return this;
	}

	void HIDPS2KeyboardService::Dealloc()
	{
		Object::Dealloc();
	}


	void HIDPS2KeyboardService::HandleInterrupt(uint8_t vector)
	{
		IOAssert(vector == 0x21, "Interrupt vector must be 0x21");

		bool isKeyDown = true;
		bool is0E = false;

		while(inb(kIOPS2ControllerStatusPort) & kIOPS2ControllerStatusOutputBuffer)
		{
			uint8_t scancode = inb(kIOPS2ControllerDataPort);
			uint32_t keyCode = 0;

			switch(scancode)
			{
				case 0xf0:
					isKeyDown = false;
					break;

				case 0xe0:
					is0E = true;
					break;

				default:
				{
					if(is0E)
					{
						switch(scancode)
						{
							case 0x1f:
								keyCode = KeyCodeLeftGUI;
								break;
							case 0x14:
								keyCode = KeyCodeRightControl;
								break;
							case 0x27:
								keyCode = KeyCodeRightGUI;
								break;
							case 0x11:
								keyCode = KeyCodeRightOption;
								break;
							case 0x71:
								keyCode = KeyCodeDelete;
								break;

						}

						if(keyCode)
							break;
					}

					if(scancode >= 0x60 && keyCode == 0x0)
						break;

					keyCode = HIDPS2KeyboardTranslationTable[scancode];
					break;
				}
			}

			if(keyCode)
			{
				DispatchEvent(keyCode, isKeyDown);

				isKeyDown = true;
				is0E = false;
			}
		}
	}

	bool HIDPS2KeyboardService::Start()
	{
		if(!HIDKeyboardService::Start())
			return false;

		outb(0x64, 0xad);

		while(inb(kIOPS2ControllerStatusPort) & kIOPS2ControllerStatusOutputBuffer)
			inb(kIOPS2ControllerDataPort);

		SendCommand(0x20);
		uint8_t config = (ReadData() & ~((1 << 1) | (1 << 6)));
		SendCommand(0x60, config);

		// Perform a self test
		SendCommand(0xaa);
		if(ReadData() != 0x55)
			return false;

		SendCommand(0xae);
		register_interrupt(0x21, this, IOMemberFunctionCast(InterruptHandler, this, &HIDPS2KeyboardService::HandleInterrupt));

		return true;
	}

	void HIDPS2KeyboardService::Stop()
	{
		outb(0x64, 0xad);
		register_interrupt(0x21, this, nullptr);

		Service::Stop();
	}

	void HIDPS2KeyboardService::SendCommand(uint8_t command, uint8_t argument)
	{
		while(inb(kIOPS2ControllerStatusPort) & kIOPS2ControllerStatusInputBuffer)
		{}

		outb(kIOPS2ControllerCommandPort, command);

		while(inb(kIOPS2ControllerStatusPort) & kIOPS2ControllerStatusInputBuffer)
		{}

		outb(kIOPS2ControllerDataPort, argument);
	}

	void HIDPS2KeyboardService::SendCommand(uint8_t command)
	{
		while(inb(kIOPS2ControllerStatusPort) & kIOPS2ControllerStatusInputBuffer)
		{}

		outb(kIOPS2ControllerCommandPort, command);
	}

	uint8_t HIDPS2KeyboardService::ReadData()
	{
		while(!(inb(kIOPS2ControllerStatusPort) & kIOPS2ControllerStatusOutputBuffer))
		{}

		return inb(kIOPS2ControllerDataPort);
	}
}
