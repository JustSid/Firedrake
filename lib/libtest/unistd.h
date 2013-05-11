//
//  unistd.h
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

#ifndef _UNISTD_H_
#define _UNISTD_H_

#include "stdint.h"
#define kVFSMaxFilenameLength 256

typedef enum
{
	vfs_nodeTypeFile,
	vfs_nodeTypeDirectory,
	vfs_nodeTypeLink
} vfs_node_type_t;

typedef struct
{
	vfs_node_type_t type;
	char name[kVFSMaxFilenameLength];

	uint32_t id;
	size_t   size;

	timestamp_t atime;
	timestamp_t mtime;
	timestamp_t ctime;
} vfs_stat_t;

typedef struct vfs_directory_entry_s
{
	uint32_t id;
	char name[kVFSMaxFilenameLength];
	vfs_node_type_t type;
} vfs_directory_entry_t;


int open(const char *path, int flags);
void close(int fd);

size_t read(int fd, void *buffer, size_t count);
size_t write(int fd, const void *buffer, size_t count);
off_t lseek(int fd, off_t offset, int whence);
off_t readdir(int fd, vfs_directory_entry_t *entp, uint32_t count);

int mkdir(const char *path);
int remove(const char *path);
int move(const char *source, const char *target);
int stat(const char *path, vfs_stat_t *stat);

#endif
