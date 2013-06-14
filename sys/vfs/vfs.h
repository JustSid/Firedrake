//
//  vfs.h
//  Firedrake
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

#ifndef _VFS_H_
#define _VFS_H_

#include <prefix.h>
#include "descriptor.h"
#include "filesystem.h"
#include "node.h"
#include "path.h"
#include "context.h"
#include "fcntl.h"

struct process_s;

bool vfs_registerFilesystem(vfs_descriptor_t *descriptor);
bool vfs_unregisterFilesystem(const char *name);
vfs_descriptor_t *vfs_filesystemWithName(const char *name);

int vfs_open(vfs_context_t *context, const char *path, int flags, int *errno);
int vfs_close(vfs_context_t *context, int fd);

vfs_file_t *vfs_duplicateFile(vfs_context_t *context, struct process_s *process, vfs_file_t *file, int *errno);
void vfs_forceClose(vfs_context_t *context, struct process_s *process, vfs_file_t *file);

size_t vfs_read(vfs_context_t *context, int fd, void *data, size_t size, int *errno);
size_t vfs_write(vfs_context_t *context, int fd, const void *data, size_t size, int *errno);
off_t vfs_seek(vfs_context_t *context, int fd, off_t offset, int whence, int *errno);
off_t vfs_readDir(vfs_context_t *context, int fd, struct vfs_directory_entry_s *entp, uint32_t count, int *errno);

bool vfs_mkdir(vfs_context_t *context, const char *path, int *errno);
bool vfs_remove(vfs_context_t *context, const char *path, int *errno);
bool vfs_move(vfs_context_t *context, const char *source, const char *destination, int *errno);
bool vfs_stat(vfs_context_t *context, const char *path, vfs_stat_t *stat, int *errno);

bool vfs_init(void *data);

#endif
