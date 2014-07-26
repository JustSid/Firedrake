//
//  vfs.h
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2014 by Sidney Just
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

#ifndef _VFS_H_
#define _VFS_H_

#include <prefix.h>
#include <bootstrap/multiboot.h>
#include <libc/stddef.h>
#include <libc/stdint.h>
#include <libc/sys/fcntl.h>
#include <libc/sys/types.h>
#include <libc/sys/unistd.h>
#include <libc/sys/dirent.h>
#include <kern/kern_return.h>

#include "descriptor.h"
#include "instance.h"
#include "node.h"
#include "context.h"

namespace VFS
{
	Node *GetRootNode();

	kern_return_t Open(Context *context, const char *path, int flags, int &fd);
	kern_return_t Close(Context *context, int fd);
	kern_return_t StatFile(Context *context, const char *path, stat *buf);

	kern_return_t Write(Context *context, int fd, const void *data, size_t size, size_t &written);
	kern_return_t Read(Context *context, int fd, void *data, size_t size, size_t &read);
	kern_return_t Seek(Context *context, int fd, off_t offset, int whence, off_t &result);

	kern_return_t ReadDir(Context *context, int fd, dirent *entry, size_t count, off_t &read);

	kern_return_t Init(Sys::MultibootHeader *info);
}

#endif /* _VFS_H_ */
