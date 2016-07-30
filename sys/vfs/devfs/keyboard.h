//
//  keyboard.h
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

#ifndef _DEVICES_KEYBOARD_H_
#define _DEVICES_KEYBOARD_H_

#include <prefix.h>
#include <libio/core/IOObject.h>
#include <libio/hid/IOHIDKeyboardUtilities.h>
#include <libcpp/ringbuffer.h>
#include <vfs/vfs.h>
#include <vfs/cfs/cfs_node.h>

namespace VFS
{
	namespace Devices
	{
		class Keyboard : public IO::Object
 		{
		public:
			Keyboard *Init(size_t index);

			void HandleEvent(IO::KeyboardEvent *event);

		private:
			static constexpr size_t kKeyboardMaxBufferSize = 4096;

			size_t Write(VFS::Context *context, off_t offset, const void *data, size_t size);
			size_t Read(VFS::Context *context, off_t offset, void *data, size_t size);

			CFS::Node *_node;
			std::ringbuffer<uint8_t, kKeyboardMaxBufferSize> _data;

			IODeclareMeta(Keyboard)
		};
	}
}

#endif /* _DEVICES_KEYBOARD_H_ */
