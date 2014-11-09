//
//  task.h
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

#ifndef _TASK_H_
#define _TASK_H_

#include <prefix.h>
#include <libc/stddef.h>
#include <libc/stdint.h>
#include <libc/sys/types.h>
#include <libc/sys/spinlock.h>
#include <libcpp/atomic.h>
#include <libcpp/queue.h>
#include <machine/memory/memory.h>
#include <loader/loader.h>
#include <kern/kern_return.h>
#include "thread.h"

namespace VFS
{
	class Context;
	class File;
}

namespace Core
{
	class Task
	{
	public:
		friend class Thread;
		friend class VFS::Context;

		enum class State
		{
			Waiting,
			Running,
			Blocked,
			Died
		};

		Task(); // This really is just to create the kernel task
		Task(const char *path);
		~Task();

		kern_return_t AttachThread(Thread **outthread, Thread::Entry entry, size_t stack);

		void Lock();
		void Unlock();

		pid_t GetPid() const { return _pid; }
		Sys::VM::Directory *GetDirectory() const { return _directory; }
		kern_return_t GetResult() const { return _result; }

		// VFS
		VFS::Context *GetVFSContext() const { return _context; }

		VFS::File *GetFileForDescriptor(int fd);
		void SetFileForDescriptor(VFS::File *file, int fd);

		int AllocateFileDescriptor();
		void FreeFileDescriptor(int fd);

	private:
		void RemoveThread(Thread *thread);

		std::atomic<int32_t> _tidCounter;
		Sys::VM::Directory *_directory;

		kern_return_t _result;
		Sys::Executable *_executable;

		cpp::queue<Thread> _threads;
		Thread *_mainThread;

		spinlock_t _lock;
		pid_t _pid;
		State _state;

		bool _ring3;

		VFS::Context *_context;
		VFS::File *_files[CONFIG_MAX_FILES];
	};
}

#endif /* _TASK_H_ */
