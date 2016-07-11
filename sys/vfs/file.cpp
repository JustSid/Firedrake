//
//  file.cpp
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

#include <kern/panic.h>
#include <kern/kprintf.h>
#include <libc/string.h>
#include "file.h"
#include "node.h"

namespace VFS
{
	IODefineMeta(File, IO::Object)
	IODefineMeta(FileDirectory, File)

	File *File::Init(Node *node, int flags)
	{
		if(!IO::Object::Init())
			return nullptr;

		_node   = node->Retain();
		_flags  = flags;
		_offset = 0;

		return this;
	}

	void File::Dealloc()
	{
		_node->Release();
		IO::Object::Dealloc();
	}

	void File::SetOffset(off_t offset)
	{
		_offset = offset;
	}



	FileDirectory *FileDirectory::Init(Node *node, int flags)
	{
		if(!File::Init(node, flags))
			return nullptr;

		if(!node->IsDirectory())
			panic("Node must be directory!");

		Directory *directory = static_cast<Directory *>(node);
		directory->Lock();

		const IO::Dictionary *children = directory->GetChildren();
		children->Enumerate<Node, IO::String>([&](Node *node, IO::String *filename, __unused bool &stop) {

			dirent entry;

			entry.type = static_cast<int>(node->GetType());
			entry.id   = node->GetID();

			strlcpy(entry.name, filename->GetCString(), MAXNAME);

			_entries.push_back(std::move(entry));

		});

		directory->Unlock();
		return this;
	}
}

