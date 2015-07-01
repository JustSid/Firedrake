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

namespace Sys
{
	void FinishBootstrapping();
}

namespace OS
{
	ipc_port_t port;

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

		while(1)
		{
			bool enabled = Sys::DisableInterrupts();
			Sys::CPU *current = Sys::CPU::GetCurrentCPU();

			for(size_t i = 0; i < count; i ++)
			{
				Sys::CPU *cpu = Sys::CPU::GetCPUWithID(i);
				WorkProxy *proxy = proxies + i;

				proxy->entries = (cpu == current) ? proxy->queue->PopAll() : proxy->queue->PopAllRemote();
			}

			if(enabled)
				Sys::EnableInterrupts();

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
			enabled = Sys::DisableInterrupts();
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

			if(enabled)
				Sys::EnableInterrupts();

			Sys::CPUHalt();
		}
	}

	void KernelTaskMain()
	{
		Sys::Personality::GetPersonality()->FinishBootstrapping();

		Task *self = Scheduler::GetScheduler()->GetActiveTask();

		__unused Thread *workThread = self->AttachThread(reinterpret_cast<Thread::Entry>(&KernelWorkThread), Thread::PriorityClassKernel, 16, nullptr);

		// Set up some simple IPC ports
		IPC::System *system = self->GetTaskSystem();
		system->Lock();

		IPC::Port *echoPort = system->AddPort(1, IPC::Port::Rights::Any | IPC::Port::Rights::Receive);
		echoPort->Retain();

		system->Unlock();

		kprintf("Echo port: %llu\n", echoPort->GetPortName());

		// Start the test program
		__unused Task *task = Task::Alloc()->InitWithFile(Scheduler::GetScheduler()->GetKernelTask(), "/bin/test.bin");

		// Listen for incoming IPC messages
		size_t bufferSize = 1024;

		void *buffer = kalloc(bufferSize);
		ipc_header_t *header = reinterpret_cast<ipc_header_t *>(buffer);

		header->receiver = echoPort->GetPortName();
		header->flags = IPC_HEADER_FLAG_BLOCK;
		header->size = bufferSize - sizeof(ipc_header_t);

		IPC::Message *message = IPC::Message::Alloc()->Init(header);

		while(1)
		{
			KernReturn<void> result = Read(self, message);

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

						ipc_header_t *header = reinterpret_cast<ipc_header_t *>(buffer);

						header->receiver = echoPort->GetPortName();
						header->flags = IPC_HEADER_FLAG_BLOCK;
						header->size = bufferSize - sizeof(ipc_header_t);

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
