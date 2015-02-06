//
//  unistd.c
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

#include "unistd.h"
#include "dirent.h"
#include "syscall.h"

#ifndef __KERNEL

int open(const char *path, int flags)
{
	return (int)SYSCALL2(SYS_Open, path, flags);
}
void close(int fd)
{
	SYSCALL1(SYS_Close, fd);
}


size_t read(int fd, void *buffer, size_t count)
{
	return (size_t)SYSCALL4(SYS_Read, fd, buffer, count, 0);
}
size_t write(int fd, const void *buffer, size_t count)
{
	return (size_t)SYSCALL3(SYS_Write, fd, buffer, count);
}
off_t lseek(int fd, off_t offset, int whence)
{
	return (off_t)SYSCALL3(SYS_Seek, fd, offset, whence);
}
off_t readdir(int fd, struct dirent *entp, size_t count)
{
	return (off_t)SYSCALL4(SYS_Read, fd, entp, count, 1);
}


int mkdir(__unused const char *path)
{
	return -1;
}
int remove(__unused const char *path)
{
	return -1;
}
int move(__unused const char *source, __unused const char *target)
{
	return -1;
}

int stat(const char *path, struct stat *buf)
{
	return (int)SYSCALL3(SYS_Stat, path, buf, 0);
}
int lstat(const char *path, struct stat *buf)
{
	return (int)SYSCALL3(SYS_Stat, path, buf, 1);
}


pid_t getpid()
{
	return (pid_t)SYSCALL1(SYS_Pid, 0);
}
pid_t getppid()
{
	return (pid_t)SYSCALL1(SYS_Pid, 1);
}

#endif /* __KERNEL */
