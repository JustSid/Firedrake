//
//  keyboard.cpp
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

#include <libc/drake/input.h>
#include <libc/stdio.h>
#include "keyboard.h"

namespace VFS
{
	namespace Devices
	{
		IODefineMeta(Keyboard, IO::Object)

		Keyboard *Keyboard::Init(size_t index)
		{
			if(!IO::Object::Init())
				return nullptr;

			char name[128];
			sprintf(name, "keyboard%d", (int)index);

			_node = VFS::GetDevFS()->CreateNode(name, this,
			                                    IOMemberFunctionCast(CFS::Node::ReadProc, this, &Keyboard::Read),
			                                    IOMemberFunctionCast(CFS::Node::WriteProc, this, &Keyboard::Write));

			return this;
		}

		void Keyboard::HandleEvent(IO::KeyboardEvent *event)
		{
			KeyboardBuffer buffer;

			buffer.isDown = event->IsKeyDown();
			buffer.character = event->GetCharacter();
			buffer.keyCode = event->GetKeyCode();
			buffer.modifier = event->GetModifier();

			_node->Lock();
			Write(VFS::Context::GetKernelContext(), 0, &buffer, sizeof(KeyboardBuffer));
			_node->Unlock();
		}

		size_t Keyboard::Write(__unused VFS::Context *context, __unused off_t offset, const void *data, size_t size)
		{
			if((size % sizeof(KeyboardBuffer)) != 0)
				return -1;

			uint8_t buffer[kKeyboardMaxBufferSize];
			size_t write = std::min(size, sizeof(buffer));

			if(write == 0)
				return 0;

			context->CopyDataOut(reinterpret_cast<const uint8_t *>(data) + offset, buffer, write);

			for(size_t i = 0; i < write; i ++)
				_data.push(buffer[i]);

			return write;
		}

		size_t Keyboard::Read(__unused VFS::Context *context, __unused off_t offset, void *data, size_t size)
		{
			if((size % sizeof(KeyboardBuffer)) != 0)
				return -1;

			uint8_t buffer[kKeyboardMaxBufferSize];
			size_t read = std::min(size, _data.size());

			if(read == 0)
				return 0;

			for(size_t i = 0; i < read; i ++)
			{
				if(_data.size() == 0)
					break;

				buffer[i] = _data.pop();
			}

			context->CopyDataIn(buffer, data, read);
			return read;
		}
	}
}
