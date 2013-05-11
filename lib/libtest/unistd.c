//
//  unistd.c
//  libtest
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

#include "unistd.h"
#include "syscall.h"

int open(const char *path, int flags)
{
	return (int)syscall(SYS_OPEN, path, flags);
}
void close(int fd)
{
	syscall(SYS_CLOSE, fd);
}

size_t read(int fd, void *buffer, size_t count)
{
	return (size_t)syscall(SYS_READ, fd, buffer, count);
}
size_t write(int fd, const void *buffer, size_t count)
{
	return (size_t)syscall(SYS_WRITE, fd, buffer, count);
}
off_t lseek(int fd, off_t offset, int whence)
{
	return (off_t)syscall(SYS_SEEK, fd, offset, whence);
}
off_t readdir(int fd, vfs_directory_entry_t *entp, uint32_t count)
{
	return (off_t)syscall(SYS_DIRREAD, fd, entp, count);
}

int mkdir(const char *path)
{
	return (int)syscall(SYS_MKDIR, path);
}
int remove(const char *path)
{
	return (int)syscall(SYS_REMOVE, path);
}
int move(const char *source, const char *target)
{
	return (int)syscall(SYS_MOVE, source, target);
}
int stat(const char *path, vfs_stat_t *stat)
{
	return (int)syscall(SYS_STAT, path, stat);
}
