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

#include "../sys/kern_trap.h"
#include "ipc_port.h"

#ifndef __KERNEL /* Don't move up to avoid ending up with an empty translation unit */

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

#endif /* __KERNEL */
