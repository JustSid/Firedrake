//
//  file.h
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

#ifndef _VFS_FILE_H_
#define _VFS_FILE_H_

#include <prefix.h>
#include <libc/stdint.h>
#include <libc/sys/fcntl.h>
#include <libc/sys/types.h>
#include <libcpp/vector.h>

#include "directory.h"

namespace VFS
{
	class Node;
	class File
	{
	public:
		File(Node *node, int flags);
		virtual ~File();

		Node *GetNode() const { return _node; }
		off_t GetOffset() const { return _offset; }
		int GetFlags() const { return _flags; }

		void SetOffset(off_t offset);

	private:
		Node *_node;
		off_t _offset;
		int _flags;
	};

	class FileDirectory : public File
	{
	public:
		FileDirectory(Node *node, int flags);

		size_t GetCount() const { return _entries.size(); }
		const DirectoryEntry *GetEntries() const { return _entries.data(); }

	private:
		std::vector<DirectoryEntry> _entries;
	};

	constexpr File *InvalidFile = (File *)0xffdeadff;
}

#endif /* _VFS_FILE_H_ */
