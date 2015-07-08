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
	IPC::Port *echoPort = nullptr;

	void KernelTaskMain()
	{
		Sys::Personality::GetPersonality()->FinishBootstrapping();

		Task *self = Scheduler::GetScheduler()->GetActiveTask();

		// Set up the main IPC ports
		IPC::Space *space = self->GetIPCSpace();
		space->Lock();

		echoPort = space->AllocateReceivePort();
		echoPort->Retain();

		bootstrapPort = space->AllocateReceivePort();
		bootstrapPort->Retain();

		space->Unlock();

		__unused Thread *workThread = self->AttachThread(reinterpret_cast<Thread::Entry>(&KernelWorkThread), Thread::PriorityClassKernel, 16, nullptr);
		__unused Thread *bootstrapThread = self->AttachThread(reinterpret_cast<Thread::Entry>(&BootstrapServerThread), Thread::PriorityClassKernel, 16, nullptr);

		// Start the test program
		__unused Task *task = Task::Alloc()->InitWithFile(Scheduler::GetScheduler()->GetKernelTask(), "/bin/test.bin");

		// Listen for incoming IPC messages
		size_t bufferSize = 1024;

		void *buffer = kalloc(bufferSize);
		ipc_header_t *header = reinterpret_cast<ipc_header_t *>(buffer);

		IPC::Message *message = IPC::Message::Alloc()->Init(header);

		while(1)
		{
			header->port = echoPort->GetName();
			header->flags = IPC_HEADER_FLAG_BLOCK;
			header->size = bufferSize - sizeof(ipc_header_t);

			space->Lock();
			KernReturn<void> result = space->Read(message);
			space->Unlock();

			if(result.IsValid())
			{
				char *data = reinterpret_cast<char *>(IPC_GET_DATA(header));
				kputs(data);
			}
			else
			{
				switch(result.GetError().GetCode())
				{
					case KERN_NO_MEMORY:
					{
						// Expand the buffer to allow reading the whole message
						bufferSize = message->GetHeader()->realSize + sizeof(ipc_header_t);
						kfree(buffer);

						buffer = kalloc(bufferSize);

						header = reinterpret_cast<ipc_header_t *>(buffer);

						message->Release();
						message = IPC::Message::Alloc()->Init(header);

						break;
					}

					default:
						break;
				}
			}
		}
	}
}
