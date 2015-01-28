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

	void Test();

	void KernelTaskMain()
	{
		Sys::Personality::GetPersonality()->FinishBootstrapping();

		// Set up some simple IPC ports
		Task *self = Scheduler::GetActiveTask();
		IPC::Port *echoPort = self->GetTaskSystem()->AllocatePort(1, IPC::Port::Rights::Any | IPC::Port::Rights::Receive);

		kprintf("Echo port: %llu\n", echoPort->GetPortName());

		// Start the test program
		__unused Task *task = Task::Alloc()->InitWithFile(Scheduler::GetKernelTask(), "/bin/test.bin");

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
			KernReturn<void> result = Read(message);

			if(result.IsValid())
			{
				char *data = reinterpret_cast<char *>(IPC_GET_DATA(header));
				kprintf("Echo: '%s'\n", data);
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
