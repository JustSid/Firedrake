//
//  vfs.c
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
#include <container/list.h>
#include <memory/memory.h>
#include <libc/string.h>
#include <libc/stdio.h>
#include <scheduler/process.h>
#include <system/syslog.h>
#include <system/helper.h>
#include "ffs/ffs.h"
#include "vfs.h"

static list_t *vfs_list = NULL;
static vfs_instance_t *vfs_rootInstance = NULL;

vfs_node_t *vfs_rootNode = NULL;
vfs_context_t *vfs_kernelContext = NULL;

bool vfs_registerFilesystem(vfs_descriptor_t *descriptor)
{
	assert(descriptor);
	assert(descriptor->createInstance);
	assert(descriptor->destroyInstance);

	list_lock(vfs_list);
	list_insertBack(vfs_list, descriptor);
	list_unlock(vfs_list);

	return true;
}

bool vfs_unregisterFilesystem(const char *name)
{
	assert(name);

	list_lock(vfs_list);

	vfs_descriptor_t *descriptor = list_first(vfs_list);
	while(descriptor)
	{
		if(strcmp(descriptor->name, name) == 0)
		{
			if(array_count(descriptor->instances) > 0 || (descriptor->flags & vfs_descriptorFlagPersistent))
			{
				list_unlock(vfs_list);
				return false;
			}

			// Clean the descriptor
			array_destroy(descriptor->instances);
			hfree(NULL, descriptor->name);

			// Remove the descriptor from the list
			list_remove(vfs_list, descriptor);
			list_unlock(vfs_list);

			return true;
		}

		descriptor = descriptor->next;
	}

	list_unlock(vfs_list);
	return false;
}

vfs_descriptor_t *vfs_filesystemWithName(const char *name)
{
	assert(name);

	list_lock(vfs_list);

	vfs_descriptor_t *descriptor = list_first(vfs_list);
	while(descriptor)
	{
		if(strcmp(descriptor->name, name) == 0)
		{
			list_unlock(vfs_list);
			return descriptor;
		}

		descriptor = descriptor->next;
	}

	list_unlock(vfs_list);
	return NULL;
}

// --------
// File operations
// --------

int vfs_open(vfs_context_t *context, const char *path, int flags, int *errno)
{
	process_t *process = process_getCurrentProcess();
	process_lock(process);

	int filedescriptor = process_allocateFiledescriptor(process);
	if(filedescriptor == -1)
	{
		*errno = EMFILE;
		process_unlock(process);

		return -1;
	}

	vfs_node_t *node = vfs_resolvePath(path, context, errno);

	if(node)
	{
		if(flags & O_EXCL)
		{	
			*errno = EEXIST;

			process_releaseFiledescriptor(process, filedescriptor);
			process_unlock(process);

			return -1;
		}

		vfs_instance_t *instance = node->instance;
		vfs_file_t *file = instance->callbacks.fileOpen(instance, context, node, flags, errno);
		if(!file)
		{
			process_releaseFiledescriptor(process, filedescriptor);
			process_unlock(process);

			return -1;
		}

		process_setFileForFiledescriptor(process, filedescriptor, file);
		process_unlock(process);

		return filedescriptor;
	}

	if(flags & O_CREAT)
	{
		char name[kVFSMaxFilenameLength];

		vfs_node_t *parent = vfs_resolvePathToParent(path, context, name, errno);
		vfs_instance_t *instance = parent->instance;

		if(parent)
		{
			node = instance->callbacks.createFile(instance, context, parent, name, errno);
			vfs_file_t *file = instance->callbacks.fileOpen(instance, context, node, flags, errno);

			if(!file)
			{
				process_releaseFiledescriptor(process, filedescriptor);
				process_unlock(process);

				return -1;
			}

			process_setFileForFiledescriptor(process, filedescriptor, file);
			process_unlock(process);

			return filedescriptor;
		}
	}
	
	*errno = ENOENT;

	process_releaseFiledescriptor(process, filedescriptor);
	process_unlock(process);

	return -1;
}

int vfs_close(vfs_context_t *context, int fd)
{
	process_t *process = process_getCurrentProcess();
	vfs_file_t *file = process_fileWithFiledescriptor(process, fd);
	if(!file)
		return -1;

	vfs_instance_t *instance = file->node->instance;
	instance->callbacks.fileClose(instance, context, file);
	
	process_releaseFiledescriptor(process, fd);
	return 0;
}

vfs_file_t *vfs_duplicateFile(vfs_context_t *context, process_t *process, vfs_file_t *file, int *errno)
{
	vfs_instance_t *instance = file->node->instance;

	process_lock(process);

	vfs_file_t *result = instance->callbacks.fileDuplicate(instance, context, file, errno);

	process_unlock(process);

	return result;
}

void vfs_forceClose(vfs_context_t *context, process_t *process, vfs_file_t *file)
{
	vfs_instance_t *instance = file->node->instance;
	instance->callbacks.fileClose(instance, context, file);
}

size_t vfs_write(vfs_context_t *context, int fd, const void *data, size_t size, int *errno)
{
	process_t *process = process_getCurrentProcess();
	vfs_file_t *file = process_fileWithFiledescriptor(process, fd);

	if(!file || !(file->flags & O_WRONLY || file->flags & O_RDWR))
	{
		*errno = EBADF;
		return -1;
	}

	if(file->node->type == vfs_nodeTypeDirectory)
	{
		*errno = EISDIR;
		return -1;
	}

	vfs_instance_t *instance = file->node->instance;
	return instance->callbacks.fileWrite(instance, context, file, data, size, errno);
}

size_t vfs_read(vfs_context_t *context, int fd, void *data, size_t size, int *errno)
{
	process_t *process = process_getCurrentProcess();
	vfs_file_t *file = process_fileWithFiledescriptor(process, fd);

	if(!file || !(file->flags & O_RDONLY || file->flags & O_RDWR))
	{
		*errno = EBADF;
		return -1;
	}

	if(file->node->type == vfs_nodeTypeDirectory)
	{
		*errno = EISDIR;
		return -1;
	}

	vfs_instance_t *instance = file->node->instance;
	return instance->callbacks.fileRead(instance, context, file, data, size, errno);
}

off_t vfs_seek(vfs_context_t *context, int fd, off_t offset, int whence, int *errno)
{
	process_t *process = process_getCurrentProcess();
	vfs_file_t *file = process_fileWithFiledescriptor(process, fd);

	if(!file)
	{
		*errno = EBADF;
		return -1;
	}

	vfs_instance_t *instance = file->node->instance;
	return instance->callbacks.fileSeek(instance, context, file, offset, whence, errno);
}

off_t vfs_readDir(vfs_context_t *context, int fd, struct vfs_directory_entry_s *entp, uint32_t count, int *errno)
{
	process_t *process = process_getCurrentProcess();
	vfs_file_t *file = process_fileWithFiledescriptor(process, fd);

	if(!file)
	{
		*errno = EBADF;
		return -1;
	}

	if(file->node->type == vfs_nodeTypeFile)
	{
		*errno = ENOTDIR;
		return -1;
	}


	vfs_instance_t *instance = file->node->instance;
	return instance->callbacks.dirRead(instance, context, file, entp, count, errno);
}


bool vfs_mkdir(vfs_context_t *context, const char *path, int *errno)
{
	char name[kVFSMaxFilenameLength];
	vfs_node_t *node = vfs_resolvePathToParent(path, context, name, errno);

	if(!node)
	{
		*errno = ENOENT;
		return false;
	}

	if(node->type != vfs_nodeTypeDirectory)
	{
		*errno = EINVAL;
		return false;
	}

	vfs_instance_t *instance = node->instance;
	node = instance->callbacks.createDirectory(instance, context, node, name, errno);

	return (node != NULL);
}

bool vfs_remove(vfs_context_t *context, const char *path, int *errno)
{
	vfs_node_t *node = vfs_resolvePath(path, context, errno);

	if(!node)
		return false;

	vfs_instance_t *instance = node->instance;
	bool result = instance->callbacks.nodeRemove(instance, context, node, errno);

	return result;
}

bool vfs_move(vfs_context_t *context, const char *source, const char *destination, int *errno)
{
	vfs_instance_t *instance;
	vfs_node_t *node;
	vfs_node_t *target;

	node = vfs_resolvePath(source, context, errno);
	if(!node)
		return false;

	target = vfs_resolvePath(destination, context, errno);
	instance = node->instance;

	if(!target)
	{
		char name[kVFSMaxFilenameLength];
		target = vfs_resolvePathToParent(destination, context, name, errno);
		if(!target)
			return false;

		bool result = instance->callbacks.nodeMove(instance, context, node, target, name, errno);
		return result;
	}
	
	bool result = instance->callbacks.nodeMove(instance, context, node, target, NULL, errno);
	return result;
}

bool vfs_stat(vfs_context_t *context, const char *path, vfs_stat_t *stat, int *errno)
{
	vfs_node_t *node = vfs_resolvePath(path, context, errno);

	if(!node)
	{
		*errno = ENOENT;
		return false;
	}

	vfs_instance_t *instance = node->instance;
	vfs_stat_t buf;

	bool result = instance->callbacks.nodeStat(instance, context, node, &buf, errno);
	if(result)
	{
		int error;
		vfs_contextCopyDataIn(context, &buf, sizeof(vfs_stat_t), stat, &error);

		return true;
	}

	return false;
}

// --------
// File instance operations
// --------

vfs_instance_t *vfs_createInstanceWithName(const char *name)
{
	vfs_descriptor_t *descriptor = vfs_filesystemWithName(name);
	return descriptor ? descriptor->createInstance(descriptor) : NULL;
}

void vfs_readInitrd()
{
	struct multiboot_module_s *modules = (struct multiboot_module_s *)bootinfo->mods_addr;
	for(uint32_t i=0; i<bootinfo->mods_count; i++)
	{
		struct multiboot_module_s *module = &modules[i];
		if(strcmp(module->name, "initrd") == 0)
		{
			uint8_t *buffer = (uint8_t *)module->start;
			uint8_t *end = (uint8_t *)module->end;

			dbg("loading initrd (%u bytes)...", end - buffer);

			while(buffer < end)
			{
				// Read the file header
				uint32_t nameLength;
				uint32_t binaryLength;

				memcpy(&nameLength, buffer + 0, 4);
				memcpy(&binaryLength, buffer + 4, 4);
				buffer += 8;

				// Read the files destination path
				char *name = halloc(NULL, nameLength + 1);
				memcpy(name, buffer, nameLength);

				name[nameLength] = '\0';
				buffer += nameLength;

				// Read the files data
				int error;
				int fd = vfs_open(vfs_getKernelContext(), name, O_WRONLY | O_CREAT, &error);
				if(fd >= 0)
				{
					size_t left = binaryLength;
					while(left > 0)
					{
						size_t written = vfs_write(vfs_getKernelContext(), fd, buffer, left, &error);

						left   -= written;
						buffer += written;
					}

					vfs_close(vfs_getKernelContext(), fd);
				}
				else
				{
					buffer += binaryLength;
					warn("Couldn't open and write %s, reason: %i\n", name, error);
				}

				hfree(NULL, name);
			}

			// TODO: At this point we could unmap the initrd module from the virtual address space

			dbg("done");
			break;
		}
	}
}

extern void kern_loadKernelData();

bool vfs_init(__unused void *data)
{
	vfs_list = list_create(sizeof(vfs_descriptor_t), offsetof(vfs_descriptor_t, next), offsetof(vfs_descriptor_t, prev));

	if(!vfs_list || !ffs_init())
		return false;

	vfs_rootInstance  = vfs_createInstanceWithName("ffs");
	vfs_rootNode      = vfs_rootInstance->root;
	vfs_kernelContext = vfs_contextCreate(vm_getKernelDirectory());

	// The scheduler is already running and has spanwed the kerneld process
	// Time to fixup its context
	process_t *process = process_getCurrentProcess();
	process->context = vfs_kernelContext;

	int error;

	vfs_mkdir(vfs_getKernelContext(), "/bin", &error);
	vfs_mkdir(vfs_getKernelContext(), "/lib", &error);
	vfs_mkdir(vfs_getKernelContext(), "/etc", &error);

	vfs_readInitrd();
	kern_loadKernelData(); // Signal the kernel that it's now safe to load the kernel string and symbol table

	return true;
}
