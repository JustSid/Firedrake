//
//  IOHIDKeyboardService.cpp
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

#include "IOHIDKeyboardService.h"

extern "C" void __libkern_dispatchKeyboardEvent(IO::KeyboardEvent *event);
extern "C" IO::Object *__libkern_registerKeyboard(IO::Object *service);
extern "C" void __libkern_unregisterKeyboard(IO::Object *service);

namespace IO
{
	IODefineMeta(HIDKeyboardService, Service)

	HIDKeyboardService *HIDKeyboardService::InitWithProperties(Dictionary *properties)
	{
		if(!Service::InitWithProperties(properties))
			return nullptr;

		_modifier = 0;
		_keyboard = nullptr;

		return this;
	}

	void HIDKeyboardService::Start()
	{
		Service::Start();

		_keyboard = __libkern_registerKeyboard(this);
		_keyboard->Retain();
	}

	void HIDKeyboardService::Stop()
	{
		Service::Stop();

		__libkern_unregisterKeyboard(this);

		_keyboard->Release();
		_keyboard = nullptr;
	}

	void HIDKeyboardService::DispatchEvent(uint32_t keyCode, bool keyDown)
	{
		if(keyCode >= KeyCodeRightShift && keyCode <= KeyCodeLeftGUI)
		{
			uint16_t modifier = static_cast<uint16_t>((1 << keyCode));

			if(keyDown)
				_modifier |= modifier;
			else
				_modifier &= ~modifier;
		}

		KeyboardEvent event(this, keyCode, _modifier, keyDown);
		__libkern_dispatchKeyboardEvent(&event);
	}
}
