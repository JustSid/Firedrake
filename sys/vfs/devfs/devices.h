//
//  devices.h
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

#ifndef _DEVICES_H_
#define _DEVICES_H_

#include <prefix.h>
#include "pty.h"
#include "keyboard.h"
#include "framebuffer.h"

namespace VFS
{
	namespace Devices
	{
		// Keyboards
		Keyboard *GetKeyboard(IO::Object *service);

		Keyboard *RegisterKeyboard(IO::Object *service);
		void UnregisterKeyboard(IO::Object *service);

		// Framebuffer
		Framebuffer *GetFramebuffer(IO::Framebuffer *framebuffer);

		Framebuffer *RegisterFramebuffer(IO::Framebuffer *framebuffer);
		void UnregisterFramebuffer(IO::Framebuffer *framebuffer);

		KernReturn<void> Init();
	}
}

#endif /* _DEVICES_H_ */
