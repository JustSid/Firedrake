//
//  task.cpp
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

#include <kern/kprintf.h>
#include <kern/panic.h>
#include <libc/string.h>
#include <vfs/file.h>
#include <vfs/vfs.h>
#include <machine/interrupts/trampoline.h>
#include <machine/debug.h>
#include <os/waitqueue.h>
#include "scheduler.h"
#include "task.h"

namespace OS
{
	static std::atomic<pid_t> _taskPidCounter;
	IODefineMeta(Task, IO::Object)

	extern IPC::Port *echoPort;
	extern IPC::Port *bootstrapPort;

	Task::Task() :
		schedulerEntry(this)
	{}

	KernReturn<Task *> Task::Init(Task *parent)
	{
		if(!IO::Object::Init())
			return Error(KERN_FAILURE);

		spinlock_init(&_lock);

		_parent = parent;
		_state = State::Running;
		_pid = _taskPidCounter.fetch_add(1) + 1;
		_ring3 = false;
		_tidCounter = 1;
		_mainThread = nullptr;
		_directory = Sys::VM::Directory::GetKernelDirectory();
		_context = nullptr;
		_fileCounter = 0;
		_files = IO::Dictionary::Alloc()->Init();
		_executable = nullptr;
		_threads = IO::Array::Alloc()->Init();
		_name = nullptr;
		_exitedThreads = 0;

		_space = IPC::Space::Alloc()->Init();
		_taskPort = _space->AllocateReceivePort();

		if(echoPort)
		{
			_specialPorts[IPC_SPECIAL_PORT_KERNEL] = nullptr;
			_specialPorts[IPC_SPECIAL_PORT_BOOTSTRAP] = _space->AllocateSendPort(bootstrapPort, IPC::Port::Right::Send, IPC_PORT_NULL);
			_specialPorts[IPC_SPECIAL_PORT_PUT] = _space->AllocateSendPort(echoPort, IPC::Port::Right::Send, IPC_PORT_NULL);
		}

		Scheduler::GetScheduler()->AddTask(this);
		return this;
	}

	KernReturn<Task *> Task::InitWithFile(Task *parent, const char *path)
	{
		KernReturn<Task *> result = Init(parent);
		if(!result.IsValid())
			return result.GetError();

		IO::String *name = IO::String::Alloc()->InitWithCString(path);
		SetName(name);
		name->Release();

		_ring3 = true;
		_directory = nullptr;

		KernReturn<Sys::VM::Directory *> directory;
		if((directory = Sys::VM::Directory::Create()).IsValid() == false)
			return directory.GetError();

		_directory = directory;
		_context = new VFS::Context(this, _directory, VFS::GetRootNode());

		if(!_context)
			return Error(KERN_NO_MEMORY);

		KernReturn<Executable *> executable;
		if((executable = Executable::Alloc()->Init(_directory, path)).IsValid() == false)
			return executable.GetError();

		_executable = executable;

		Sys::TrampolineMapIntoDirectory(_directory);

		KernReturn<Thread *> thread = AttachThread(static_cast<Thread::Entry>(_executable->GetEntry()), Thread::PriorityClass::PriorityClassNormal, 0, nullptr);
		if(!thread.IsValid())
			return thread.GetError();

		return this;
	}


	void Task::Dealloc()
	{
		_executable->Release();

		if(_directory != Sys::VM::Directory::GetKernelDirectory())
			delete _directory;

		_files->Release();
		_threads->Release();

		IO::SafeRelease(_name);

		IO::Object::Dealloc();
	}


	void Task::SetName(IO::String *name)
	{
		IO::SafeRelease(_name);
		_name = IO::SafeRetain(name);
	}

	void Task::PronounceDead(int32_t exitCode)
	{
		_exitCode = exitCode;
		_state = State::Died;
	}

	KernReturn<Thread *> Task::AttachThread(Thread::Entry entry, Thread::PriorityClass priority, size_t stack, IO::Array *parameters)
	{
		spinlock_lock(&_lock);

		KernReturn<Thread *> thread = Thread::Alloc()->Init(this, entry, priority, stack, parameters);

		if(!thread.IsValid())
		{
			spinlock_unlock(&_lock);
			return thread;
		}

		if(!_mainThread)
			_mainThread = thread;

		_threads->AddObject(thread);
		spinlock_unlock(&_lock);
		
		Scheduler::GetScheduler()->AddThread(thread);
		return thread;
	}

	void Task::MarkThreadExit(Thread *thread)
	{
		_exitedThreads ++;
		Wakeup(thread->GetJoinToken());
	}

	void Task::RemoveThread(Thread *thread)
	{
		thread->_task = nullptr;

		spinlock_lock(&_lock);
		_threads->RemoveObject(thread);
		spinlock_unlock(&_lock);
	}


	void Task::Lock()
	{
		spinlock_lock(&_lock);
	}
	void Task::Unlock()
	{
		spinlock_unlock(&_lock);
	}


	Thread *Task::GetThreadWithID(tid_t id)
	{
		Lock();

		Thread *result = nullptr;

		_threads->Enumerate<Thread>([&](Thread *thread, size_t index, bool &stop) {

			if(thread->GetTid() == id)
			{
				result = thread;
				stop = true;
			}

		});

		Unlock();

		return result;
	}

	VFS::File *Task::GetFileForDescriptor(int fd)
	{
		IO::Number *lookup = IO::Number::Alloc()->InitWithInt32(fd);

		Lock();
		VFS::File *file = _files->GetObjectForKey<VFS::File>(lookup);
		Unlock();

		lookup->Release();
		return file;
	}
	void Task::SetFileForDescriptor(VFS::File *file, int fd)
	{
		IO::Number *lookup = IO::Number::Alloc()->InitWithInt32(fd);

		Lock();

		if(file)
			_files->SetObjectForKey(file, lookup);
		else
			_files->SetObjectForKey(IO::Null::GetNull(), lookup);

		Unlock();

		lookup->Release();
	}


	int Task::AllocateFileDescriptor()
	{
		int fd = (_fileCounter ++);

		Lock();

		IO::Number *lookup = IO::Number::Alloc()->InitWithInt32(fd);
		_files->SetObjectForKey(IO::Null::GetNull(), lookup);
		lookup->Release();

		Unlock();

		return fd;
	}
	void Task::FreeFileDescriptor(int fd)
	{
		Lock();

		IO::Number *lookup = IO::Number::Alloc()->InitWithInt32(fd);
		_files->RemoveObjectForKey(lookup);
		lookup->Release();

		Unlock();
	}
}
