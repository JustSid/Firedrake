//
//  node.h
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

#ifndef _VFS_NODE_H_
#define _VFS_NODE_H_

#include <prefix.h>
#include <system/lock.h>
#include <system/time.h>
#include <container/hashset.h>

#define kVFSMaxFilenameLength 256

struct vfs_directory_s;
struct vfs_instance_s;
struct vfs_context_s;

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

typedef struct vfs_node_s
{
	char name[kVFSMaxFilenameLength];
	struct vfs_instance_s *instance;
	vfs_node_type_t type;

	spinlock_t lock;
	uint32_t references;
	uint32_t id;
	size_t size;

	timestamp_t atime;
	timestamp_t mtime;
	timestamp_t ctime;

	struct vfs_directory_s *parent;
	void *data;
} vfs_node_t;

typedef struct vfs_directory_s
{
	vfs_node_t node;

	hashset_t *childs;
} vfs_directory_t;

typedef struct vfs_link_s
{
	vfs_node_t node;

	vfs_node_t *target;
} vfs_link_t;

// TODO: That should probably be put somewhere else since it's a wrapper over an open node
typedef struct vfs_directory_entry_s
{
	uint32_t id;
	char name[kVFSMaxFilenameLength];
	vfs_node_type_t type;
} vfs_directory_entry_t;

typedef struct vfs_file_s
{
	vfs_node_t *node;
	size_t offset;
	int flags;
	void *data;
} vfs_file_t;

vfs_file_t *vfs_fileCreate(vfs_node_t *node, int flags, void *data, int *errno);
void vfs_fileDelete(vfs_file_t *file);


void vfs_nodeLock(vfs_node_t *node);
void vfs_nodeUnlock(vfs_node_t *node);

// These methods don't lock the node, appropriate locking must be done by the caller!
vfs_node_t *vfs_nodeCreate(struct vfs_instance_s *instance, const char *name, vfs_node_type_t type, void *data);
vfs_node_t *vfs_linkCreate(struct vfs_instance_s *instance, const char *name, vfs_node_t *target, void *data);
void vfs_nodeDelete(vfs_node_t *node);

void vfs_nodeRetain(vfs_node_t *node);
void vfs_nodeRelease(vfs_node_t *node);

bool vfs_directoryAttachNode(vfs_directory_t *directory, vfs_node_t *node);
bool vfs_directoryRemoveNode(vfs_directory_t *directory, vfs_node_t *node);

vfs_node_t *vfs_nodeWithName(vfs_node_t *node, struct vfs_context_s *context, const char *name, int *errno);
bool vfs_nodeStat(vfs_node_t *node, struct vfs_context_s *context, vfs_stat_t *stat, int *errno);

#endif
