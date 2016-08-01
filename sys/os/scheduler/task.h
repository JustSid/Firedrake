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
#include <libcpp/intrusive_list.h>
#include <machine/memory/memory.h>
#include <kern/kern_return.h>
#include <libio/core/IOObject.h>
#include <libio/core/IOArray.h>
#include <libio/core/IOString.h>
#include <libio/core/IODictionary.h>
#include <libio/core/IONumber.h>
#include <libio/core/IONull.h>
#include <os/ipc/IPC.h>
#include <os/syscall/syscall_mmap.h>
#include <os/loader/loader.h>

#include "thread.h"

namespace VFS
{
	class Context;
	class File;
}

namespace OS
{
	class Task : public IO::Object
	{
	public:
		friend class Thread;
		friend class VFS::Context;

		enum class State : uint8_t
		{
			Running,
			Died
		};

		KernReturn<Task *> Init(Task *parent);
		KernReturn<Task *> InitWithFile(Task *parent, const char *path);

		KernReturn<Thread *> AttachThread(Thread::Entry entry, Thread::PriorityClass priority, size_t stack, IO::Array *parameters);
		void RemoveThread(Thread *thread);
		void MarkThreadExit(Thread *thread);

		void PronounceDead(int32_t exitCode);
		void SetName(IO::String *name);

		void Lock();
		void Unlock();

		Task *GetParent() const { return _parent; }

		pid_t GetPid() const { return _pid; }
		Sys::VM::Directory *GetDirectory() const { return _directory; }
		int GetNice() const { return _nice.load(); }
		Thread *GetMainThread() const { return _mainThread; }
		Thread *GetThreadWithID(tid_t id);
		State GetState() const { return _state.load(); }

		IO::String *GetName() const { return _name; }

		// VFS
		VFS::Context *GetVFSContext() const { return _context; }

		// Must be called with the task lock being held!
		VFS::File *GetFileForDescriptor(int fd);
		void SetFileForDescriptor(VFS::File *file, int fd);

		// Must also be called with the task lock being held
		int AllocateFileDescriptor();
		void FreeFileDescriptor(int fd);

		// IPC
		IPC::Space *GetIPCSpace() const { return _space; }
		IPC::Port *GetTaskPort() const { return _taskSendPort; }
		IPC::Port *GetSpecialPort(int port) const { return _specialPorts[port]; }

		// Scheduler
		std::intrusive_list<Task>::member schedulerEntry;

		// Mmap
		std::intrusive_list<MmapTaskEntry> mmapList;

	protected:
		Task();
		void Dealloc() override;

	private:
		void CheckLifecycle();

		Task *_parent;
		Sys::VM::Directory *_directory;
		Executable *_executable;

		std::atomic<int32_t> _tidCounter;
		IO::Array *_threads;
		Thread *_mainThread;

		IO::String *_name;
		int32_t _exitCode;
		std::atomic<uint32_t> _exitedThreads;

		spinlock_t _lock;
		pid_t _pid;
		std::atomic<State> _state;
		std::atomic<int> _nice;

		bool _ring3;

		VFS::Context *_context;
		IO::Dictionary *_files;
		std::atomic<int32_t> _fileCounter;

		IPC::Space *_space;
		IPC::Port *_taskPort;
		IPC::Port *_taskSendPort;
		IPC::Port *_specialPorts[__IPC_SPECIAL_PORT_MAX];

		IODeclareMeta(Task)
	};
}

#endif /* _TASK_H_ */
