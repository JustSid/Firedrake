//
//  ipc_port.h
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

#ifndef _IPC_IPC_PORT_H_
#define _IPC_IPC_PORT_H_

#include "ipc_types.h"
#include "../sys/types.h"

__BEGIN_DECLS

#define IPC_PORT_NULL ((ipc_port_t)0)
#define IPC_PORT_DEAD (~(ipc_port_t)0)

#define IPC_PORT_RIGHT_RECEIVE   0
#define IPC_PORT_RIGHT_SEND      1
#define IPC_PORT_RIGHT_SEND_ONCE 2

#define IPC_SPECIAL_PORT_HOST      0
#define IPC_SPECIAL_PORT_BOOTSTRAP 1

#define __IPC_SPECIAL_PORT_MAX 3

ipc_return_t ipc_task_space(ipc_space_t *space, pid_t pid);
ipc_port_t ipc_task_port();
ipc_port_t ipc_thread_port();
ipc_port_t ipc_get_special_port(int port);

ipc_return_t ipc_allocate_port(ipc_port_t *port);
ipc_return_t ipc_insert_port(ipc_space_t space, ipc_port_t target, ipc_port_t port, int right);

ipc_return_t ipc_deallocate_port(ipc_port_t port);

__END_DECLS

#endif /* _IPC_IPC_PORT_H_ */
