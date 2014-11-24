//
//  ffs_node.h
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

#ifndef _FFS_NODE_H_
#define _FFS_NODE_H_

#include <prefix.h>
#include <vfs/node.h>
#include <vfs/instance.h>
#include <libc/sys/fcntl.h>
#include <libc/sys/types.h>

namespace FFS
{
	class Node : public VFS::Node
	{
	public:
		Node *Init(const char *name, VFS::Instance *instance, uint64_t id);

		KernReturn<size_t> WriteData(VFS::Context *context, off_t offset, const void *data, size_t size);
		KernReturn<size_t> ReadData(VFS::Context *context, off_t offset, void *data, size_t size);

	protected:
		void Dealloc() override;

	private:
		KernReturn<void> AllocateMemory(size_t minimumSize);

		char *_data;
		off_t _size;
		size_t _pages;

		IODeclareMeta(Node)
	};
}

#endif /* _FFS_NODE_H_ */
