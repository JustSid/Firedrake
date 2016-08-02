//
//  devices.cpp
//  Firedrake
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

#include <os/pty.h>
#include "devices.h"

namespace VFS
{
	namespace Devices
	{
		/* Keyboards */
		static IO::Dictionary *_keyboardMap;
		static std::atomic<size_t> _keyboardCounter = 0;

		Keyboard *GetKeyboard(IO::Object *service)
		{
			return _keyboardMap->GetObjectForKey<Keyboard>(service);
		}

		Keyboard *RegisterKeyboard(IO::Object *service)
		{
			Keyboard *keyboard = _keyboardMap->GetObjectForKey<Keyboard>(service);
			if(keyboard)
				return keyboard;

			size_t index = (_keyboardCounter ++);
			keyboard = Keyboard::Alloc()->Init(index);

			if(keyboard)
			{
				_keyboardMap->SetObjectForKey(keyboard, service);
				keyboard->Release();
			}

			return keyboard;
		}
		void UnregisterKeyboard(IO::Object *service)
		{
			_keyboardMap->RemoveObjectForKey(service);
		}

		/* Framebuffers */
		static IO::Dictionary *_framebufferMap;
		static std::atomic<size_t> _framebufferCounter = 0;

		Framebuffer *GetFramebuffer(IO::Framebuffer *framebuffer)
		{
			return _framebufferMap->GetObjectForKey<Framebuffer>(framebuffer);
		}

		Framebuffer *RegisterFramebuffer(IO::Framebuffer *source)
		{
			Framebuffer *framebuffer = _framebufferMap->GetObjectForKey<Framebuffer>(source);
			if(framebuffer)
				return framebuffer;

			size_t index = (_framebufferCounter ++);
			framebuffer = Framebuffer::Alloc()->Init(index, source);

			if(framebuffer)
			{
				_framebufferMap->SetObjectForKey(framebuffer, source);
				framebuffer->Release();
			}

			return framebuffer;
		}
		void UnregisterFramebuffer(IO::Framebuffer *framebuffer)
		{
			_framebufferMap->RemoveObjectForKey(framebuffer);
		}

		// PTY's
		static IO::Array *_ptys = nullptr;

		KernReturn<void> Init()
		{
			_keyboardMap = IO::Dictionary::Alloc()->Init();
			_framebufferMap = IO::Dictionary::Alloc()->Init();
			_ptys = IO::Array::Alloc()->Init();

			// Create 10 PTY's
			for(size_t i = 0; i < 10; i ++)
			{
				OS::PTY *pty = OS::PTY::Alloc()->Init();
				if(pty)
					_ptys->AddObject(pty);
			}

			return ErrorNone;
		}
	}
}
