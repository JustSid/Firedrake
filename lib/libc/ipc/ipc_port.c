//
//  ipc_port.c
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

#include "ipc_port.h"

#ifndef __LIBKERN
#include "../sys/kern_trap.h"

ipc_return_t ipc_task_space(ipc_space_t *space, pid_t pid)
{
	return (ipc_return_t)KERN_TRAP2(KERN_IPC_TaskSpace, space, pid);
}

ipc_port_t ipc_task_port()
{
	ipc_port_t result;
	KERN_TRAP1(KERN_IPC_TaskPort, &result);

	return result;
}
ipc_port_t ipc_thread_port()
{
	ipc_port_t result;
	KERN_TRAP1(KERN_IPC_ThreadPort, &result);

	return result;
}
ipc_port_t ipc_get_special_port(int port)
{
	ipc_port_t result = IPC_PORT_NULL;
	KERN_TRAP2(KERN_IPC_GetSpecialPort, &result, port);

	return result;
}

ipc_return_t ipc_allocate_port(ipc_port_t *port)
{
	return (ipc_return_t)KERN_TRAP1(KERN_IPC_AllocatePort, port);
}
ipc_return_t ipc_insert_port(ipc_space_t space, ipc_port_t target, ipc_port_t port, int right)
{
	return (ipc_return_t)KERN_TRAP4(KERN_IPC_InsertPort, space, target, port, right);
}

ipc_return_t ipc_deallocate_port(ipc_port_t port)
{
	return (ipc_return_t)KERN_TRAP1(KERN_IPC_DeallocatePort, port);
}

#else
#include <libkern.h>

ipc_port_t ipc_get_special_port(__unused int port)
{
	panic("ipc_get_special_port() called");
}

ipc_return_t ipc_allocate_port(__unused ipc_port_t *port)
{
	panic("ipc_allocate_port() called");
}
ipc_return_t ipc_deallocate_port(__unused ipc_port_t port)
{
	panic("ipc_deallocate_port() called");
}

#endif
