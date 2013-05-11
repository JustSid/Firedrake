//
//  ffs.c
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

#include <vfs/vfs.h>
#include <scheduler/process.h>
#include <memory/memory.h>
#include "ffs.h"
#include "ffs_callbacks.h"
#include "ffs_node.h"

void ffs_associateDataWithFile(int fd, const void *data, size_t size)
{
	process_t *process = process_getCurrentProcess();
	vfs_file_t *file = process_fileWithFiledescriptor(process, fd);
	ffs_node_data_t *node = file->node->data;

	node->data = (uint8_t *)data;
	node->size = size;
	node->pages = pageCount(size);

	file->node->size = size;
}


ffs_instance_data_t *ffs_createInstanceData()
{
	ffs_instance_data_t *data = halloc(NULL, sizeof(ffs_instance_data_t));
	if(data)
	{
		data->nodes = hashset_create(100, hash_integer, hash_integerCompare);
	}

	return data;
}


void ffs_insertNode(vfs_instance_t *instance, vfs_node_t *node)
{
	ffs_instance_data_t *data = instance->data;
	hashset_setObjectForKey(data->nodes, node, (const void *)node->id);
}

void ffs_removeNode(vfs_instance_t *instance, vfs_node_t *node)
{
	ffs_instance_data_t *data = instance->data;
	hashset_removeObjectForKey(data->nodes, (const void *)node->id);
}

vfs_node_t *ffs_lookupNode(vfs_instance_t *instance, uint32_t id)
{
	ffs_instance_data_t *data = instance->data;
	return hashset_objectForKey(data->nodes, (const void *)id);
}


vfs_instance_t *ffs_createInstance(vfs_descriptor_t *descriptor)
{
	vfs_callbacks_t callbacks;
	callbacks.createFile = ffs_createFile;
	callbacks.createDirectory = ffs_createDirectory;
	callbacks.createLink  = ffs_createLink;
	callbacks.destroyNode = ffs_destroyNode;

	callbacks.lookupNode  = ffs_lookupNode;
	callbacks.accessNode  = ffs_accessNode;

	callbacks.nodeRemove = ffs_nodeRemove;
	callbacks.nodeMove = ffs_nodeMove;
	callbacks.nodeStat = ffs_nodeStat;

	callbacks.fileOpen = ffs_fileOpen;
	callbacks.fileClose = ffs_fileClose;
	callbacks.fileWrite = ffs_fileWrite;
	callbacks.fileRead = ffs_fileRead;
	callbacks.fileSeek = ffs_fileSeek;
	callbacks.dirRead = ffs_dirRead;

	ffs_instance_data_t *data = ffs_createInstanceData();
	vfs_instance_t *instance = vfs_instanceCreate(descriptor, &callbacks, data);
	return instance;
}

void ffs_destroyInstance(__unused vfs_instance_t *instance)
{
}



bool ffs_init()
{
	vfs_descriptor_t *descriptor = vfs_descriptorCreate("ffs", vfs_descriptorFlagPersistent);
	descriptor->createInstance  = ffs_createInstance;
	descriptor->destroyInstance = ffs_destroyInstance;

	return vfs_registerFilesystem(descriptor);
}
