//
//  node.c
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

#include <errno.h>
#include <memory/memory.h>
#include <libc/string.h>
#include "node.h"
#include "fcntl.h"
#include "descriptor.h"
#include "filesystem.h"

vfs_file_t *vfs_fileCreate(vfs_node_t *node, int flags, void *data, int *errno)
{
	if(!(flags & O_RDONLY) && !(flags & O_WRONLY) && !(flags & O_RDWR))
	{
		*errno = EINVAL;
		return NULL;
	}

	if(node->type == vfs_nodeTypeDirectory && (flags & O_WRONLY || flags & O_RDWR))
	{
		*errno = EINVAL;
		return NULL;
	}

	vfs_file_t *file = halloc(NULL, sizeof(vfs_file_t));
	if(file)
	{
		vfs_nodeRetain(node);

		file->node  = node;
		file->flags = flags;
		file->offset = 0;
		file->data = data;

		if(node->type == vfs_nodeTypeFile && (flags & O_APPEND))
		{
			file->offset = node->size;
		}
	}

	return file;
}

void vfs_fileDelete(vfs_file_t *file)
{
	vfs_nodeRelease(file->node);
	hfree(NULL, file);
}



vfs_node_t *vfs_nodeFresh(struct vfs_instance_s *instance, const char *name, size_t size)
{
	assert(name);
	assert(instance);
	assert(size >= sizeof(vfs_node_t));

	spinlock_lock(&instance->lock);

	vfs_node_t *node = halloc(NULL, size);
	strcpy(node->name, name); // TODO: Some sanity checks would probably be helpful

	node->instance = instance;
	node->id = instance->lastID ++;
	node->references = 1;
	node->parent = NULL;
	node->size = 0;
	node->atime = node->mtime = node->ctime = time_getTimestamp();

	spinlock_unlock(&instance->lock);

	return node;
}

vfs_node_t *vfs_nodeCreate(struct vfs_instance_s *instance, const char *name, vfs_node_type_t type, void *data)
{
	vfs_node_t *node = NULL;

	switch(type)
	{
		case vfs_nodeTypeFile:
		{
			node = vfs_nodeFresh(instance, name, sizeof(vfs_node_t));
			break;
		}

		case vfs_nodeTypeDirectory:
		{
			vfs_directory_t *directory = (vfs_directory_t *)vfs_nodeFresh(instance, name, sizeof(vfs_directory_t));
			directory->childs = hashset_create(10, hash_cstring, hash_cstringCompare);

			node = (vfs_node_t *)directory;
			break;
		}

		default:
			panic("vfs_nodeCreate() doesn't support type %i", (int)type);
			break;
	}

	node->data = data;
	node->type = type;

	return node;
}

vfs_node_t *vfs_linkCreate(struct vfs_instance_s *instance, const char *name, vfs_node_t *target, void *data)
{
	vfs_link_t *link = (vfs_link_t *)vfs_nodeFresh(instance, name, sizeof(vfs_link_t));

	link->node.type = vfs_nodeTypeLink;
	link->node.data = data;

	link->target = target;

	vfs_nodeRetain(target);

	return (vfs_node_t *)link;
}

void vfs_nodeDelete(vfs_node_t *node)
{
	switch(node->type)
	{
		case vfs_nodeTypeDirectory:
		{
			vfs_directory_t *directory = (vfs_directory_t *)node;
			iterator_t *iterator = hashset_iterator(directory->childs);

			while((node = iterator_nextObject(iterator)))
			{
				node->parent = NULL;
				vfs_nodeRelease(node);
			}

			iterator_destroy(iterator);
			hashset_destroy(directory->childs);
			break;
		}

		case vfs_nodeTypeLink:
		{
			vfs_link_t *link = (vfs_link_t *)node;
			vfs_nodeRelease(link->target);
			break;
		}

		default:
			break;
	}

	hfree(NULL, node);
}



void vfs_nodeLock(vfs_node_t *node)
{
	spinlock_lock(&node->lock);
}

void vfs_nodeUnlock(vfs_node_t *node)
{
	spinlock_unlock(&node->lock);
}

void vfs_nodeRetain(vfs_node_t *node)
{
	node->references ++;
}

void vfs_nodeRelease(vfs_node_t *node)
{
	if((-- node->references) == 0)
	{
		node->instance->callbacks.destroyNode(node->instance, node);
	}
}


bool vfs_directoryAttachNode(vfs_directory_t *directory, vfs_node_t *node)
{
	if(node->parent)
		return false;

	void *temp = hashset_objectForKey(directory->childs, node->name);
	if(temp)
		return false;

	hashset_setObjectForKey(directory->childs, node, node->name);
	vfs_nodeRetain(node);

	node->parent = directory;

	return true;
}

bool vfs_directoryRemoveNode(vfs_directory_t *directory, vfs_node_t *node)
{
	if(node->parent == directory)
	{
		hashset_removeObjectForKey(directory->childs, node->name);
		node->parent = NULL;

		vfs_nodeRelease(node);
		return true;
	}

	return false;
}


vfs_node_t *vfs_nodeWithName(vfs_node_t *node, vfs_context_t *context, const char *name, int *errno)
{
	if(node->type == vfs_nodeTypeDirectory)
	{
		vfs_directory_t *directory = (vfs_directory_t *)node;
		vfs_node_t *child = hashset_objectForKey(directory->childs, name);

		return child;
	}

	if(node->type == vfs_nodeTypeLink)
	{
		vfs_link_t *link = (vfs_link_t *)node;
		vfs_node_t *target = link->target;

		if(!target)
		{
			if(errno)
				*errno = ENOTDIR; // Actually... What the fuck is that invalid link doing here?!

			return NULL;
		}

		return target->instance->callbacks.accessNode(target->instance, context, node, name, errno);
	}

	if(errno)
		*errno = ENOTDIR;

	return NULL;
}

bool vfs_nodeStat(vfs_node_t *node, __unused vfs_context_t *context, vfs_stat_t *stat, __unused int *errno)
{
	strcpy(stat->name, node->name);

	stat->type = node->type;
	stat->id = node->id;
	stat->size = 0;

	stat->atime = node->atime;
	stat->mtime = node->mtime;
	stat->ctime = node->ctime;

	return true;
}
