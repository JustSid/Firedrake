//
//  framebuffer.cpp
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

#include <libc/stdio.h>
#include "framebuffer.h"

namespace VFS
{
	namespace Devices
	{
		IODefineMeta(Framebuffer, IO::Object)

		Framebuffer *Framebuffer::Init(size_t index, IO::Framebuffer *source)
		{
			if(!IO::Object::Init())
				return nullptr;

			char name[128];
			sprintf(name, "framebuffer%d", (int)index);

			_source = source;
			_node = VFS::GetDevFS()->CreateNode(name, this,
			                                    IOMemberFunctionCast(CFS::Node::ReadProc, this, &Framebuffer::Read),
			                                    IOMemberFunctionCast(CFS::Node::WriteProc, this, &Framebuffer::Write));

			_node->SetSizeProc(IOMemberFunctionCast(CFS::Node::SizeProc, this, &Framebuffer::GetSize));

			return this;
		}

		void Framebuffer::Dealloc()
		{
			IO::Object::Dealloc();
		}


		size_t Framebuffer::Write(VFS::Context *context, __unused off_t offset, const void *data, size_t size)
		{
			uint8_t *memory = static_cast<uint8_t *>(_source->GetMemory());
			context->CopyDataOut(data, memory, size);

			return size;
		}
		size_t Framebuffer::Read(VFS::Context *context, off_t offset, void *data, size_t size)
		{
			const uint8_t *memory = static_cast<const uint8_t *>(_source->GetMemory()) + offset;
			context->CopyDataIn(memory, data, size);

			return size;
		}
		size_t Framebuffer::GetSize() const
		{
			return _source->GetWidth() * _source->GetHeight() * 4; // ARGB 32bit
		}
	}
}
