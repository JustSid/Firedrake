//
//  ffs_callbacks.c
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
#include <libc/string.h>
#include <libc/math.h>
#include <memory/memory.h>
#include <system/syslog.h>
#include "ffs_callbacks.h"
#include "ffs_node.h"

void ffs_insertNode(vfs_instance_t *instance, vfs_node_t *node);
void ffs_removeNode(vfs_instance_t *instance, vfs_node_t *node);

bool ffs_attachNode(vfs_node_t *parent, vfs_node_t *node, int *errno)
{
	vfs_directory_t *directory = (vfs_directory_t *)parent;
	vfs_nodeLock(parent);

	if(hashset_objectForKey(directory->childs, node->name))
	{
		vfs_nodeUnlock(parent);

		*errno = EEXIST;
		return false;
	}

	vfs_directoryAttachNode(directory, node);
	vfs_nodeUnlock(parent);

	return true;
}


vfs_node_t *ffs_createFile(vfs_instance_t *instance, __unused vfs_context_t *context, vfs_node_t *parent, const char *name, int *errno)
{
	vfs_node_t *node = vfs_nodeCreate(instance, name, vfs_nodeTypeFile, ffs_node_dataCreate());

	if(parent)
	{
		if(!ffs_attachNode(parent, node, errno))
		{
			vfs_nodeDelete(node);
			return NULL;
		}
	}

	ffs_insertNode(instance, node);
	return node;
}
vfs_node_t *ffs_createDirectory(vfs_instance_t *instance, __unused vfs_context_t *context, vfs_node_t *parent, const char *name, int *errno)
{
	vfs_node_t *node = vfs_nodeCreate(instance, name, vfs_nodeTypeDirectory, NULL);
	if(parent)
	{
		if(!ffs_attachNode(parent, node, errno))
		{
			vfs_nodeDelete(node);
			return NULL;
		}
	}

	ffs_insertNode(instance, node);
	return node;
}
vfs_link_t *ffs_createLink(vfs_instance_t *instance, __unused vfs_context_t *context, vfs_node_t *parent, vfs_node_t *target, const char *name, int *errno)
{
	vfs_link_t *link = (vfs_link_t *)vfs_linkCreate(instance, name, target, NULL);
	
	if(parent)
	{
		if(!ffs_attachNode(parent, (vfs_node_t *)link, errno))
		{
			vfs_nodeDelete((vfs_node_t *)link);
			return NULL;
		}
	}

	ffs_insertNode(instance, (vfs_node_t *)link);
	return link;
}

void ffs_destroyNode(vfs_instance_t *instance, vfs_node_t *node)
{
	void *data = node->data;
	if(node->type == vfs_nodeTypeFile && data)
		ffs_node_dataDestroy(data);

	ffs_removeNode(instance, node);
	vfs_nodeDelete(node);
}


vfs_node_t *ffs_accessNode(__unused vfs_instance_t *instance, vfs_context_t *context, vfs_node_t *parent, const char *name, int *errno)
{
	vfs_nodeLock(parent);
	vfs_node_t *node = vfs_nodeWithName(parent, context, name, errno);
	vfs_nodeUnlock(parent);

	if(node && node->type == vfs_nodeTypeLink)
	{
		vfs_link_t *link = (vfs_link_t *)node;
		node = link->target;
	}

	return node;
}


bool ffs_nodeRemove(__unused vfs_instance_t *instance, __unused vfs_context_t *context, vfs_node_t *node, __unused int *errno)
{
	vfs_directory_t *directory = node->parent;

	vfs_nodeLock((vfs_node_t *)directory);
	bool result = vfs_directoryRemoveNode(directory, node);
	vfs_nodeUnlock((vfs_node_t *)directory);

	return result;
}

bool ffs_nodeMove(__unused vfs_instance_t *instance, __unused vfs_context_t *context, vfs_node_t *node, vfs_node_t *newParent, const char *name, int *errno)
{
	vfs_directory_t *directory = node->parent;
	vfs_directory_t *parent = (vfs_directory_t *)newParent;

	vfs_nodeRetain(node);
	vfs_nodeLock((vfs_node_t *)directory);

	bool rename = (name != NULL);
	name = name ? name : node->name;

	vfs_node_t *current = hashset_objectForKey(parent->childs, (void *)name);
	if(current)
	{
		if(current->type == vfs_nodeTypeDirectory && hashset_count(((vfs_directory_t *)current)->childs) > 0)
		{
			*errno = ENOTEMPTY;

			vfs_nodeUnlock((vfs_node_t *)directory);
			vfs_nodeRelease(node);

			return false;
		}

		vfs_directoryRemoveNode(parent, current);
	}

	vfs_directoryRemoveNode(directory, node);

	if(rename)
		strcpy(node->name, name);

	vfs_directoryAttachNode(parent, node);

	vfs_nodeUnlock((vfs_node_t *)directory);
	vfs_nodeRelease(node);

	return true;
}


bool ffs_nodeStat(__unused vfs_instance_t *instance, vfs_context_t *context, vfs_node_t *node, vfs_stat_t *stat, int *errno)
{
	bool result = vfs_nodeStat(node, context, stat, errno);
	if(result)
	{
		if(node->type == vfs_nodeTypeFile)
		{
			ffs_node_data_t *data = node->data;
			stat->size = data->size;
		}
	}

	return result;
}

typedef struct
{
	vfs_directory_entry_t *entries;
	size_t count;
	size_t pages;
} ffs_file_data_t;

vfs_file_t *ffs_fileOpen(__unused vfs_instance_t *instance, __unused vfs_context_t *context, vfs_node_t *node, int flags, int *errno)
{
	vfs_file_t *file = vfs_fileCreate(node, flags, NULL, errno);

	if(file && node->type == vfs_nodeTypeDirectory)
	{
		vfs_directory_t *directory = (vfs_directory_t *)node;
		size_t count = hashset_count(directory->childs);
		size_t pages = VM_PAGE_COUNT(count * sizeof(vfs_directory_entry_t));

		vfs_directory_entry_t *data = mm_alloc(vm_getKernelDirectory(), pages, VM_FLAGS_KERNEL);
		vfs_directory_entry_t *temp = data;

		iterator_t *iterator = hashset_iterator(directory->childs);
		vfs_node_t *entry;
		while((entry = iterator_nextObject(iterator)))
		{
			strcpy(temp->name, entry->name);
			temp->type = entry->type;
			temp->id = entry->id;

			temp ++;
		}

		ffs_file_data_t *ffsData = halloc(NULL, sizeof(ffs_file_data_t));
		ffsData->entries = data;
		ffsData->pages   = pages;
		ffsData->count   = count;

		file->data = ffsData;
	}

	if(flags & O_TRUNC)
	{
		ffs_node_data_t *data = node->data;
		if(data)
		{
			data->size = 0;
			node->size = 0;
		}
	}

	return file;
}
vfs_file_t *ffs_fileDuplicate(__unused vfs_instance_t *instance, __unused vfs_context_t *context, vfs_file_t *file, int *errno)
{
	//vfs_file_t *copy = vfs_fileCreate(node, flags, NULL, errno);
	return NULL;
}
void ffs_fileClose(__unused vfs_instance_t *instance, __unused vfs_context_t *context, vfs_file_t *file)
{
	if(file->data)
	{
		ffs_file_data_t *data = file->data;
		mm_free(data->entries, vm_getKernelDirectory(), data->pages);
		hfree(NULL, data);
	}

	vfs_fileDelete(file);
}


size_t ffs_fileWrite(__unused vfs_instance_t *instance, vfs_context_t *context, vfs_file_t *file, const void *data, size_t size, int *errno)
{
	vfs_nodeLock(file->node);
	ffs_node_data_t *node = file->node->data;

	size_t written = ffs_node_writeData(node, context, file->offset, data, size, errno);
	if(written != (size_t)-1)
	{
		file->node->size = node->size;
		file->offset += written;
	}

	vfs_nodeUnlock(file->node);

	return written;
}
size_t ffs_fileRead(__unused vfs_instance_t *instance, vfs_context_t *context, vfs_file_t *file, void *data, size_t size, int *errno)
{
	vfs_nodeLock(file->node);
	ffs_node_data_t *node = file->node->data;

	size_t read = ffs_node_readData(node, context, file->offset, data, size, errno);
	if(read != (size_t)-1)
	{
		file->offset += read;
	}
	else
	{
		panic("Failed with errno %i\n", *errno);
	}

	vfs_nodeUnlock(file->node);
	return read;
}

off_t ffs_fileSeek(__unused vfs_instance_t *instance, __unused vfs_context_t *context, vfs_file_t *file, off_t offset, int whence, int *errno)
{
	vfs_nodeLock(file->node);
	ffs_node_data_t *node = file->node->data;

	off_t moved = -1;
	size_t toffset = 0;
	switch(whence)
	{
		case SEEK_SET:
			toffset = offset;
			break;

		case SEEK_CUR:
			toffset = file->offset + offset;
			break;

		case SEEK_END:
			toffset = node->size + offset;
			break;

		default:
			*errno = EINVAL;
			return -1;
	}

	if(toffset <= node->size)
	{
		file->offset = toffset;
		moved = (off_t)file->offset;
	}

	vfs_nodeUnlock(file->node);
	return moved;
}

off_t ffs_dirRead(__unused vfs_instance_t *instance, vfs_context_t *context, vfs_file_t *file, vfs_directory_entry_t *entp, uint32_t count, int *errno)
{
	vfs_nodeLock(file->node);
	ffs_file_data_t *node = file->data;
	vfs_directory_entry_t *entries = node->entries + file->offset;

	off_t read = 0;
	uint32_t left = node->count - file->offset;
	count = MIN(left, count);
	
	if(count > 0)
	{
		bool result = vfs_contextCopyDataIn(context, entries, count * sizeof(vfs_directory_entry_t), entp, errno);
		read = -1;

		if(result)
		{
			file->offset += count;
			read = count;
		}
	}

	vfs_nodeUnlock(file->node);
	return read;
}
