//
//  sys/mman.c
//  libc
//
//  Created by Sidney Just
//  Copyright (c) 2013 by Sidney Just
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

#include "mman.h"
#include "syscall.h"

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	unsigned int result = syscall(SYS_MMAP, addr, length, prot, flags, fd, offset);
	return (void *)result;
}

int munmap(void *address, size_t length)
{
	unsigned int result = syscall(SYS_MUNMAP, address, length);
	return (int)result;
}

int mprotect(void *address, size_t length, int prot)
{
	unsigned int result = syscall(SYS_MPROTECT, address, length, prot);
	return (int)result;
}
