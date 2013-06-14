//
//  filesystem.h
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

#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

#include <prefix.h>
#include <system/lock.h>

#include "node.h"
#include "descriptor.h"
#include "context.h"
#include "fcntl.h"

struct vfs_descriptor_s;
struct vfs_instance_s;
struct vfs_context_s;
struct vfs_file_s;
struct vfs_directory_entry_s;

typedef struct vfs_callbacks_s
{
	vfs_node_t *(*createFile)(struct vfs_instance_s *instance, struct vfs_context_s *context, vfs_node_t *parent, const char *name, int *errno);
	vfs_node_t *(*createDirectory)(struct vfs_instance_s *instance, struct vfs_context_s *context, vfs_node_t *parent, const char *name, int *errno);
	vfs_link_t *(*createLink)(struct vfs_instance_s *instance, struct vfs_context_s *context, vfs_node_t *parent, vfs_node_t *target, const char *name, int *errno);
	void (*destroyNode)(struct vfs_instance_s *instance, vfs_node_t *node);

	vfs_node_t *(*lookupNode)(struct vfs_instance_s *instance, uint32_t id);
	vfs_node_t *(*accessNode)(struct vfs_instance_s *instance, struct vfs_context_s *context, vfs_node_t *parent, const char *name, int *errno);

	bool (*nodeRemove)(struct vfs_instance_s *instance, struct vfs_context_s *context, vfs_node_t *node, int *errno);
	bool (*nodeMove)(struct vfs_instance_s *instance, struct vfs_context_s *context, vfs_node_t *node, vfs_node_t *newParent, const char *name, int *errno);
	bool (*nodeStat)(struct vfs_instance_s *instance, struct vfs_context_s *context, vfs_node_t *node, vfs_stat_t *stat, int *errno);

	struct vfs_file_s *(*fileOpen)(struct vfs_instance_s *instance, struct vfs_context_s *context, vfs_node_t *node, int flags, int *errno);
	struct vfs_file_s *(*fileDuplicate)(struct vfs_instance_s *instance, struct vfs_context_s *contrext, struct vfs_file_s *file, int *errno);
	void (*fileClose)(struct vfs_instance_s *instance, struct vfs_context_s *context, struct vfs_file_s *file);

	size_t (*fileWrite)(struct vfs_instance_s *instance, struct vfs_context_s *context, struct vfs_file_s *file, const void *data, size_t size, int *errno);
	size_t (*fileRead)(struct vfs_instance_s *instance, struct vfs_context_s *context, struct vfs_file_s *file, void *data, size_t size, int *errno);
	off_t (*fileSeek)(struct vfs_instance_s *instance, struct vfs_context_s *context, struct vfs_file_s *file, off_t offset, int whence, int *errno);
	off_t (*dirRead)(struct vfs_instance_s *instance, struct vfs_context_s *context, struct vfs_file_s *file, struct vfs_directory_entry_s *entp, uint32_t count, int *errno);
} vfs_callbacks_t;

typedef struct vfs_instance_s
{
	spinlock_t lock;
	uint64_t lastID;

	vfs_callbacks_t callbacks;
	struct vfs_descriptor_s *descriptor;

	vfs_node_t *root;
	vfs_link_t *mountPoint; // Points to the root directory

	void *data;
} vfs_instance_t;

vfs_instance_t *vfs_instanceCreate(vfs_descriptor_t *descriptor, vfs_callbacks_t *callbacks, void *data);

#endif
