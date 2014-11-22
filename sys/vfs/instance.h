//
//  instance.h
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

#ifndef _VFS_INSTANCE_H_
#define _VFS_INSTANCE_H_

#include <prefix.h>
#include <libc/sys/spinlock.h>
#include <kern/kern_return.h>
#include <libc/stddef.h>
#include <libc/stdint.h>
#include <libc/sys/unistd.h>
#include <libc/sys/dirent.h>
#include <libc/sys/fcntl.h>
#include <libc/sys/types.h>
#include <libcpp/atomic.h>

#include "node.h"

namespace VFS
{
	class Node;
	class Context;
	class File;

	class Instance
	{
	public:
		virtual ~Instance();

		void Lock();
		void Unlock();

		virtual void CleanNode(Node *node);

		virtual KernReturn<Node *> CreateFile(Context *context, Node *parent, const char *name) = 0;
		virtual KernReturn<Node *> CreateDirectory(Context *context, Node *parent, const char *name) = 0;
		virtual KernReturn<void> DeleteNode(Context *context, Node *node) = 0;

		virtual KernReturn<Node *> LookUpNode(Context *context, uint64_t id) = 0;
		virtual KernReturn<Node *> LookUpNode(Context *context, Node *node, const char *name) = 0;


		virtual KernReturn<File *> OpenFile(Context *context, Node *node, int flags) = 0;
		virtual void CloseFile(Context *context, File *file) = 0;

		virtual KernReturn<size_t> FileRead(Context *context, File *file, void *data, size_t size) = 0;
		virtual KernReturn<size_t> FileWrite(Context *context, File *file, const void *data, size_t size) = 0;
		virtual KernReturn<off_t> FileSeek(Context *context, File *file, off_t offset, int whence) = 0;

		virtual KernReturn<void> FileStat(stat *buf, Context *context, Node *node) = 0;
		virtual KernReturn<off_t> DirRead(dirent *entry, Context *context, File *file, size_t count) = 0;

		Node *GetRootNode() const { return _rootNode; }

	protected:
		Instance();

		uint64_t GetFreeID();
		void Initialize(Node *rootNode);

		spinlock_t _lock;

	private:
		std::atomic<uint64_t> _lastID;
		Node *_rootNode;
	};
}

#endif /* _VFS_INSTANCE_H_ */
