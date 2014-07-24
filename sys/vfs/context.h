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
#include <kern/spinlock.h>

namespace VFS
{
	class Node;
	class Context
	{
	public:
		Context(Sys::VM::Directory *directory, Node *currentDir);
		~Context();

		Node *GetCurrentDir(); // Returns retained node!
		Node *GetRootDir(); // Returns retained node!

		kern_return_t SetCurrentDir(Node *currentDir);
		kern_return_t SetRootDir(Node *rootDir);

		kern_return_t CopyDataOut(const void *data, void *target, size_t length);
		kern_return_t CopyDataIn(const void *data, void *target, size_t length);

		static Context *GetKernelContext();
		static Context *GetActiveContext();

	private:
		spinlock_t _lock;

		Node *_currentDir;
		Node *_root;
		Sys::VM::Directory *_directory;
	};
}

#endif /* _VFS_CONTEXT_H_ */
