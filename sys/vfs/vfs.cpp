//
//  vfs.cpp
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2014 by Sidney Just
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

#include <kern/spinlock.h>
#include <kern/kalloc.h>
#include <kern/kprintf.h>
#include <libc/string.h>
#include <libcpp/new.h>
#include <libcpp/vector.h>
#include <kern/scheduler/scheduler.h>

#include "vfs.h"
#include "path.h"
#include "file.h"

#include <vfs/ffs/ffs_descriptor.h>

namespace VFS
{
	static spinlock_t _descriptorLock = SPINLOCK_INIT;
	static std::vector<Descriptor *> *_descriptors;

	static Instance *_rootInstance = nullptr;
	static Node *_rootNode = nullptr;

	Node *GetRootNode()
	{
		return _rootNode;
	}


	kern_return_t MakeDirectory(const char *path, Context *context)
	{
		Path resolver(path, context);

		while(resolver.GetElementsLeft() > 1)
		{
			kern_return_t result = resolver.ResolveElement();

			if(result != KERN_SUCCESS)
				return result;
		}

		Node *node = resolver.GetCurrentNode();
		if(!node->IsDirectory())
			return KERN_INVALID_ARGUMENT;
		
		node->Retain();

		Filename name = resolver.GetNextName();

		if(resolver.ResolveElement() == KERN_SUCCESS)
		{
			node->Release();
			return KERN_RESOURCE_EXISTS;
		}


		Node *directory;
		Instance *instance = node->GetInstance();
		kern_return_t result = instance->CreateDirectory(directory, context, node, name.c_str());

		node->Release();

		return result;
	}

	kern_return_t Open(Context *context, const char *path, int flags, int &fd)
	{
		Core::Task *task = Core::Scheduler::GetActiveTask();

		fd = task->AllocateFileDescriptor();
		if(fd == -1)
			return KERN_NO_MEMORY;

		Path resolver(path, context);

		while(resolver.GetElementsLeft() > 1)
		{
			kern_return_t result = resolver.ResolveElement();

			if(result != KERN_SUCCESS)
			{
				task->FreeFileDescriptor(fd);
				return result;
			}
		}

		Node *parent = resolver.GetCurrentNode();

		kern_return_t result = (resolver.GetElementsLeft() > 0) ? resolver.ResolveElement() : KERN_SUCCESS;
		Node *node = (result == KERN_SUCCESS) ? resolver.GetCurrentNode() : nullptr;

		if(node)
		{
			if(flags & O_EXCL)
			{
				task->FreeFileDescriptor(fd);
				return KERN_RESOURCE_EXISTS;
			}

			Instance *instance = node->GetInstance();
			File *file;

			result = instance->OpenFile(file, context, node, flags);
			if(result != KERN_SUCCESS)
			{
				task->FreeFileDescriptor(fd);
				return result;
			}

			task->SetFileForDescriptor(file, fd);

			return KERN_SUCCESS;
		}

		if(resolver.GetTotalElements() == 0)
		{
			task->FreeFileDescriptor(fd);
			return KERN_INVALID_ARGUMENT;
		}

		if(flags & O_CREAT)
		{
			File *file;
			Instance *instance = parent->GetInstance();

			result = instance->CreateFile(node, context, parent, resolver.GetCurrentName().c_str());
			if(result != KERN_SUCCESS)
			{	
				task->FreeFileDescriptor(fd);
				return result;
			}


			result = instance->OpenFile(file, context, node, flags);
			if(result != KERN_SUCCESS)
			{
				task->FreeFileDescriptor(fd);
				return result;
			}

			task->SetFileForDescriptor(file, fd);
			return KERN_SUCCESS;
		}

		task->FreeFileDescriptor(fd);
		return KERN_RESOURCES_MISSING;
	}
	kern_return_t Close(Context *context, int fd)
	{
		Core::Task *task = Core::Scheduler::GetActiveTask();
		File *file = task->GetFileForDescriptor(fd);

		if(!file)
			return KERN_INVALID_ARGUMENT;

		Node *node = file->GetNode();
		Instance *instance = node->GetInstance();

		instance->CloseFile(context, file);
		task->FreeFileDescriptor(fd);

		return KERN_SUCCESS;
	}


	kern_return_t Write(Context *context, int fd, const void *data, size_t size, size_t &written)
	{
		Core::Task *task = Core::Scheduler::GetActiveTask();
		File *file = task->GetFileForDescriptor(fd);

		if(!file)
			return KERN_INVALID_ARGUMENT;

		if(!(file->GetFlags() & O_WRONLY || file->GetFlags() & O_RDWR))
			return KERN_INVALID_ARGUMENT;
		
		Node *node = file->GetNode();
		if(node->IsDirectory())
			return KERN_INVALID_ARGUMENT;

		Instance *instance = node->GetInstance();
		return instance->FileWrite(written, context, file, data, size);
	}

	kern_return_t Read(Context *context, int fd, void *data, size_t size, size_t &read)
	{
		Core::Task *task = Core::Scheduler::GetActiveTask();
		File *file = task->GetFileForDescriptor(fd);

		if(!file)
			return KERN_INVALID_ARGUMENT;

		if(!(file->GetFlags() & O_RDONLY || file->GetFlags() & O_RDWR))
			return KERN_INVALID_ARGUMENT;
		
		Node *node = file->GetNode();
		if(node->IsDirectory())
			return KERN_INVALID_ARGUMENT;

		Instance *instance = node->GetInstance();
		return instance->FileRead(read, context, file, data, size);
	}

	kern_return_t ReadDir(Context *context, int fd, dirent *entry, size_t count, off_t &read)
	{
		Core::Task *task = Core::Scheduler::GetActiveTask();
		File *file = task->GetFileForDescriptor(fd);

		if(!file)
			return KERN_INVALID_ARGUMENT;

		if(!(file->GetFlags() & O_RDONLY || file->GetFlags() & O_RDWR))
			return KERN_INVALID_ARGUMENT;
		
		Node *node = file->GetNode();
		Instance *instance = node->GetInstance();

		return instance->DirRead(read, entry, context, file, count);
	}

	kern_return_t Seek(Context *context, int fd, off_t offset, int whence, off_t &result)
	{
		Core::Task *task = Core::Scheduler::GetActiveTask();
		File *file = task->GetFileForDescriptor(fd);

		if(!file)
			return KERN_INVALID_ARGUMENT;

		Node *node = file->GetNode();
		Instance *instance = node->GetInstance();

		return instance->FileSeek(result, context, file, offset, whence);
	}

	kern_return_t StatFile(Context *context, const char *path, stat *buf)
	{
		Path resolver(path, context);

		while(resolver.GetElementsLeft() > 0)
		{
			kern_return_t result = resolver.ResolveElement();

			if(result != KERN_SUCCESS)
				return result;
		}

		Node *node = resolver.GetCurrentNode();
		Instance *instance = node->GetInstance();

		struct stat temp;

		kern_return_t result = instance->FileStat(&temp, context, node);
		if(result == KERN_SUCCESS)
		{
			result = context->CopyDataIn(&temp, buf, sizeof(struct stat));
			return result;
		}

		return result;
	}


	void RegisterDescriptor(Descriptor *descriptor)
	{
		spinlock_lock(&_descriptorLock);
		_descriptors->push_back(descriptor);
		spinlock_unlock(&_descriptorLock);
	}
	void UnregisterDescriptor(__unused Descriptor *descriptor)
	{}


	kern_return_t LoadInitrdModule(Sys::MultibootModule *module)
	{
		uint8_t *buffer = reinterpret_cast<uint8_t *>(module->start);
		uint8_t *end    = reinterpret_cast<uint8_t *>(module->end);

		Context *context = Context::GetActiveContext();

		kprintf("loading initrd (%u bytes)... ", end - buffer);

		while(buffer < end)
		{
			// Read the file header
			uint32_t nameLength, binaryLength;

			memcpy(&nameLength, buffer + 0, 4);
			memcpy(&binaryLength, buffer + 4, 4);

			buffer += 8;

			// Read the filename
			char *name = static_cast<char *>(kalloc(nameLength + 1));
			memcpy(name, buffer, nameLength);

			name[nameLength] = '\0';
			buffer += nameLength;

			// Create the file
			int fd;
			kern_return_t result = Open(context, name, O_WRONLY | O_CREAT, fd);

			if(result == KERN_SUCCESS)
			{
				size_t left = binaryLength;
				while(left > 0)
				{
					size_t written;
					result = Write(context, fd, buffer, left, written);

					if(result != KERN_SUCCESS)
					{
						kprintf("Failed to write %s\n", name);

						buffer += left;
						break;
					}

					left   -= written;
					buffer += written;
				}

				Close(context, fd);
			}
			else
			{
				kprintf("Couldn't open %s to write\n", name);
				buffer += binaryLength;
			}

			kfree(name);
		}

		kprintf("done");
		return KERN_SUCCESS;
	}

	kern_return_t Init(Sys::MultibootHeader *info)
	{
		void *buffer = kalloc(sizeof(std::vector<Descriptor *>));
		if(!buffer)
			return KERN_NO_MEMORY;

		_descriptors = new(buffer) std::vector<Descriptor *>();

		// Create the default file systems
		kern_return_t result;
		Descriptor *descriptor;

		if((result = FFS::Descriptor::CreateAndRegister(descriptor)) != KERN_SUCCESS)
		{
			kprintf("Failed to register FFS");
			return result;
		}

		// Create the root file system
		if((result = descriptor->CreateInstance(_rootInstance)) != KERN_SUCCESS)
		{
			kprintf("Failed to create root instance");
			return result;
		}

		_rootNode = _rootInstance->GetRootNode();

		Context::GetKernelContext(); // Initializes the kernel context and associates it with the kernel task

		// Create some basic folders
		if((result = MakeDirectory("/usr", Context::GetKernelContext())) != KERN_SUCCESS)
			return result;
		if((result = MakeDirectory("/bin", Context::GetKernelContext())) != KERN_SUCCESS)
			return result;
		if((result = MakeDirectory("/etc", Context::GetKernelContext())) != KERN_SUCCESS)
			return result;
		if((result = MakeDirectory("/lib", Context::GetKernelContext())) != KERN_SUCCESS)
			return result;


		// Find the initrd module
		Sys::MultibootModule *module = info->modules;
		for(size_t i = 0; i < info->modulesCount; i ++)
		{
			if(strcmp((char *)module->name, "initrd") == 0)
			{
				result = LoadInitrdModule(module);

				if(result != KERN_SUCCESS)
					return result;

				break;
			}

			module = module->GetNext();
		}

		return KERN_SUCCESS;
	}
}
