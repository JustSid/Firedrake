//
//  kerneltask.cpp
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2015 by Sidney Just
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

#include <prefix.h>
#include <personality/personality.h>
#include <machine/interrupts/interrupts.h>
#include <os/scheduler/scheduler.h>
#include <os/waitqueue.h>
#include <os/ipc/IPC.h>
#include <os/locks/mutex.h>
#include <libc/ipc/ipc_message.h>

namespace Sys
{
	void FinishBootstrapping();
}

namespace OS
{
	void KernelWorkThread()
	{
		struct WorkProxy
		{
			WorkQueue::Entry *entries;
			WorkQueue *queue;
		};

		size_t count = Sys::CPU::GetCPUCount();
		WorkProxy *proxies = new WorkProxy[count];

		for(size_t i = 0; i < count; i ++)
		{
			Sys::CPU *cpu = Sys::CPU::GetCPUWithID(i);
			WorkProxy *proxy = proxies + i;

			proxy->queue = cpu->GetWorkQueue();	
		}

		Mutex lock;
		Thread *self = Scheduler::GetScheduler()->GetActiveThread();

		while(1)
		{
			lock.Lock(Mutex::Mode::NoInterrupts);
			Sys::CPU *current = Sys::CPU::GetCurrentCPU();

			for(size_t i = 0; i < count; i ++)
			{
				Sys::CPU *cpu = Sys::CPU::GetCPUWithID(i);
				WorkProxy *proxy = proxies + i;

				proxy->entries = (cpu == current) ? proxy->queue->PopAll() : proxy->queue->PopAllRemote();
			}

			lock.Unlock();

			// Perform the work
			for(size_t i = 0; i < count; i ++)
			{
				WorkProxy *proxy = proxies + i;

				WorkQueue::Entry *entry = proxy->entries;
				while(entry)
				{
					entry->callback(entry->context);
					entry = entry->next;
				}
			}

			// Refurbish the work lists
			lock.Lock(Mutex::Mode::NoInterrupts);
			current = Sys::CPU::GetCurrentCPU();

			for(size_t i = 0; i < count; i ++)
			{
				Sys::CPU *cpu = Sys::CPU::GetCPUWithID(i);
				WorkProxy *proxy = proxies + i;

				if(!proxy->entries)
					continue;

				if(cpu == current)
				{
					proxy->queue->RefurbishList(proxy->entries);
					proxy->entries = nullptr;
				}
				else
				{
					proxy->queue->RefurbishListRemote(proxy->entries);
					proxy->entries = nullptr;
				}
			}

			lock.Unlock();
			Scheduler::GetScheduler()->YieldThread(self);
		}
	}

	extern void BootstrapServerThread();

	IPC::Port *bootstrapPort = nullptr;
	IPC::Port *hostPort = nullptr;

	void __KernelPortCallback(__unused IPC::Port *port, IPC::Message *message)
	{
		ipc_header_t *header = message->GetHeader();
		switch(header->id)
		{
			case 0:
				knputs(message->GetData<char>(), header->size);
				break;

			default:
				break;
		}
	}

	void KernelTaskMain()
	{
		Sys::Personality::GetPersonality()->FinishBootstrapping();

		Task *self = Scheduler::GetScheduler()->GetActiveTask();
		Thread *thread = self->GetMainThread();

		// Set up the main IPC ports
		IPC::Space *space = self->GetIPCSpace();
		space->Lock();

		hostPort = space->AllocateCallbackPort(&__KernelPortCallback);
		hostPort->Retain();

		bootstrapPort = space->AllocateReceivePort();
		bootstrapPort->Retain();

		space->Unlock();

		__unused Thread *workThread = self->AttachThread(reinterpret_cast<Thread::Entry>(&KernelWorkThread), Thread::PriorityClassKernel, 16, nullptr);
		__unused Thread *bootstrapThread = self->AttachThread(reinterpret_cast<Thread::Entry>(&BootstrapServerThread), Thread::PriorityClassKernel, 16, nullptr);

		// Start the test program
		//__unused Task *task1 = Task::Alloc()->InitWithFile(Scheduler::GetScheduler()->GetKernelTask(), "/bin/test.bin");
		//__unused Task *task2 = Task::Alloc()->InitWithFile(Scheduler::GetScheduler()->GetKernelTask(), "/bin/test_server.bin");

		while(1)
		{
			Scheduler::GetScheduler()->YieldThread(thread);
		}
	}
}
