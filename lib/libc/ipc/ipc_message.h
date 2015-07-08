//
//  ipc_message.h
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

#ifndef _IPC_IPC_MESSAGE_H_
#define _IPC_IPC_MESSAGE_H_

#include "../sys/cdefs.h"
#include "ipc_types.h"

__BEGIN_DECLS

#define IPC_WRITE 0
#define IPC_READ  1

#define IPC_HEADER_FLAG_BLOCK (1 << 0)
#define IPC_HEADER_FLAG_RESPONSE (1 << 1)

#define IPC_HEADER_FLAG_GET_RESPONSE_BITS(flags) ((flags & 0x30000) >> 16)
#define IPC_HEADER_FLAG_RESPONSE_BITS(response) (response << 16)

#define IPC_MESSAGE_RIGHT_MOVE_SEND      0
#define IPC_MESSAGE_RIGHT_MOVE_SEND_ONCE 1
#define IPC_MESSAGE_RIGHT_COPY_SEND      2
#define IPC_MESSAGE_RIGHT_COPY_SEND_ONCE 3

typedef struct
{
	ipc_port_t port;
	ipc_port_t reply; // If IPC_HEADER_FLAG_RESPONSE is set, a port to which to send the response
	ipc_id_t id;
	ipc_bits_t flags;
	ipc_size_t size;
	ipc_size_t realSize; // Only used when reading, to signal the actual size of the packet
} ipc_header_t;

#define IPC_GET_DATA(header) (((unsigned char *)header) + sizeof(ipc_header_t))

ipc_return_t ipc_write(ipc_header_t *header);
ipc_return_t ipc_read(ipc_header_t *header);

__END_DECLS

#endif /* _IPC_IPC_MESSAGE_H_ */
