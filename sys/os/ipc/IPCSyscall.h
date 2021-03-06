//
//  IPCSyscall.h
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
#include "IPC.h"

namespace OS
{
	namespace IPC
	{
		struct IPCPortCallArgs
		{
			ipc_port_t *port;
		} __attribute__((packed));

		struct IPCTaskSpaceCallArgs
		{
			ipc_space_t *space;
			pid_t pid;
		} __attribute__((packed));

		struct IPCReadWriteArgs
		{
			ipc_header_t *header;
			ipc_size_t size;
			int mode;
		} __attribute__((packed));

		struct IPCSpecialPortArgs
		{
			ipc_port_t *result;
			int port;
		} __attribute__((packed));

		struct IPCDeallcoatePortArgs
		{
			ipc_port_t port;
		} __attribute__((packed));

		struct IPCInsertPortArgs
		{
			ipc_space_t space;
			ipc_port_t target;
			ipc_port_t port;
			int right;
		} __attribute__((packed));

		KernReturn<uint32_t> Syscall_IPCTaskPort(Thread *thread, IPCPortCallArgs *args);
		KernReturn<uint32_t> Syscall_IPCThreadPort(Thread *thread, IPCPortCallArgs *args);
		KernReturn<uint32_t> Syscall_IPCMessage(Thread *thread, IPCReadWriteArgs *args);
		KernReturn<uint32_t> Syscall_IPCAllocatePort(Thread *thread, IPCPortCallArgs *args);
		KernReturn<uint32_t> Syscall_IPCGetSpecialPort(Thread *thread, IPCSpecialPortArgs *args);
		KernReturn<uint32_t> Syscall_IPCDeallocatePort(Thread *thread, IPCDeallcoatePortArgs *args);
		KernReturn<uint32_t> Syscall_IPCTaskSpace(Thread *thread, IPCTaskSpaceCallArgs *args);
		KernReturn<uint32_t> Syscall_IPCInsertPort(Thread *thread, IPCInsertPortArgs *args);
	}
}
