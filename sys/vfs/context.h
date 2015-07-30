//
//  context.h
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

#ifndef _VFS_CONTEXT_H_
#define _VFS_CONTEXT_H_

#include <prefix.h>
#include <machine/memory/virtual.h>
#include <kern/kern_return.h>
#include <libc/sys/spinlock.h>
#include <os/scheduler/task.h>

#include <libio/IOObject.h>

namespace VFS
{
	class Node;
	class Context
	{
	public:
		Context(OS::Task *task, Sys::VM::Directory *directory, Node *currentDir);
		~Context();

		OS::Task *GetTask() const { return _task; }

		IO::StrongRef<Node> GetCurrentDir(); // Returns retained node!
		IO::StrongRef<Node> GetRootDir(); // Returns retained node!

		KernReturn<void> SetCurrentDir(Node *currentDir);
		KernReturn<void> SetRootDir(Node *rootDir);

		KernReturn<void> CopyDataOut(const void *data, void *target, size_t length);
		KernReturn<void> CopyDataIn(const void *data, void *target, size_t length);

		static Context *GetKernelContext();
		static Context *GetActiveContext();

	private:
		spinlock_t _lock;

		Node *_currentDir;
		Node *_root;
		Sys::VM::Directory *_directory;
		OS::Task *_task;
	};
}

#endif /* _VFS_CONTEXT_H_ */
