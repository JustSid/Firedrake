//
//  ipc_message.c
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

#include "ipc_message.h"

#ifndef __LIBKERN
#include "../sys/kern_trap.h"

ipc_return_t ipc_write(ipc_header_t *header)
{
	return KERN_TRAP3(KERN_IPC_Message, header, header->size, (int)IPC_WRITE);
}
ipc_return_t ipc_read(ipc_header_t *header)
{
	return KERN_TRAP3(KERN_IPC_Message, header, header->size, (int)IPC_READ);
}

#else
#include <libkern.h>

ipc_return_t ipc_write(__unused ipc_header_t *header)
{
	panic("ipc_write() called");
}
ipc_return_t ipc_read(__unused ipc_header_t *header)
{
	panic("ipc_read() called");
}

#endif
