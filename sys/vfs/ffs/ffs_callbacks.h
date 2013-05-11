//
//  ffs_callbacks.h
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

#ifndef _FFS_CALLBACKS_H_
#define _FFS_CALLBACKS_H_

#include <vfs/vfs.h>

vfs_node_t *ffs_createFile(vfs_instance_t *instance, vfs_context_t *context, vfs_node_t *parent, const char *name, int *errno);
vfs_node_t *ffs_createDirectory(vfs_instance_t *instance, vfs_context_t *context, vfs_node_t *parent, const char *name, int *errno);
vfs_link_t *ffs_createLink(vfs_instance_t *instance, vfs_context_t *context, vfs_node_t *parent, vfs_node_t *target, const char *name, int *errno);

vfs_node_t *ffs_accessNode(vfs_instance_t *instance, vfs_context_t *context, vfs_node_t *parent, const char *name, int *errno);
void ffs_destroyNode(vfs_instance_t *instance, vfs_node_t *node);

bool ffs_nodeRemove(vfs_instance_t *instance, vfs_context_t *context, vfs_node_t *node, int *errno);
bool ffs_nodeMove(vfs_instance_t *instance, vfs_context_t *context, vfs_node_t *node, vfs_node_t *newParent, const char *name, int *errno);
bool ffs_nodeStat(vfs_instance_t *instance, vfs_context_t *context, vfs_node_t *node, vfs_stat_t *stat, int *errno);

vfs_file_t *ffs_fileOpen(vfs_instance_t *, vfs_context_t *, vfs_node_t *node, int flags, int *errno);
void ffs_fileClose(vfs_instance_t *, vfs_context_t *, vfs_file_t *file);

size_t ffs_fileWrite(vfs_instance_t *, vfs_context_t *context, vfs_file_t *file, const void *data, size_t size, int *errno);
size_t ffs_fileRead(vfs_instance_t *, vfs_context_t *context, vfs_file_t *file, void *data, size_t size, int *errno);
off_t ffs_fileSeek(vfs_instance_t *, vfs_context_t *context, vfs_file_t *file, off_t offset, int whence, int *errno);
off_t ffs_dirRead(vfs_instance_t *instance, vfs_context_t *context, vfs_file_t *file, vfs_directory_entry_t *entp, uint32_t count, int *errno);

#endif
