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

#include <libc/sys/spinlock.h>
#include <kern/kalloc.h>
#include <kern/kprintf.h>
#include <libc/string.h>
#include <libcpp/new.h>
#include <libcpp/vector.h>
#include <os/scheduler/scheduler.h>

#include "vfs.h"
#include "path.h"
#include "file.h"

#include <objects/IOArray.h>
#include <vfs/ffs/ffs_descriptor.h>

#include <bootstrap/multiboot.h>

namespace VFS
{
	static spinlock_t _descriptorLock = SPINLOCK_INIT;
	static IO::Array *_descriptors;

	static Instance *_rootInstance = nullptr;
	static Node *_rootNode = nullptr;

	Node *GetRootNode()
	{
		return _rootNode;
	}


	KernReturn<void> MakeDirectory(Context *context, const char *path)
	{
		Path resolver(path, context);

		while(resolver.GetElementsLeft() > 1)
		{
			KernReturn<Node *> result = resolver.ResolveElement();

			if(!result.IsValid())
				return result.GetError();
		}

		Node *node = resolver.GetCurrentNode();
		if(!node->IsDirectory())
			return Error(KERN_INVALID_ARGUMENT);
		
		node->Retain();

		IO::StrongRef<IO::String> name = resolver.GetNextName();
		
		if(resolver.ResolveElement().IsValid())
		{
			node->Release();
			return Error(KERN_RESOURCE_EXISTS);
		}


		Instance *instance = node->GetInstance();
		KernReturn<Node *> result = instance->CreateDirectory(context, node, name->GetCString());

		node->Release();

		if(!result.IsValid())
			return result.GetError();

		return ErrorNone;
	}

	KernReturn<int> Open(Context *context, const char *path, int flags)
	{
		OS::Task *task = context->GetTask();

		int fd = task->AllocateFileDescriptor();
		if(fd == -1)
			return Error(KERN_NO_MEMORY);

		Path resolver(path, context);

		while(resolver.GetElementsLeft() > 1)
		{
			KernReturn<Node *> result = resolver.ResolveElement();

			if(result.IsValid() == false)
			{
				task->FreeFileDescriptor(fd);
				return result.GetError();
			}
		}

		Node *parent = resolver.GetCurrentNode();
		KernReturn<Node *> node = (resolver.GetElementsLeft() > 0) ? resolver.ResolveElement() : nullptr;

		if(node.IsValid() && node.Get())
		{
			if(flags & O_EXCL)
			{
				task->FreeFileDescriptor(fd);
				return Error(KERN_RESOURCE_EXISTS);
			}

			Instance *instance = node->GetInstance();

			KernReturn<File *> file = instance->OpenFile(context, node, flags);
			if(!file.IsValid())
			{
				task->FreeFileDescriptor(fd);
				return file.GetError();
			}

			task->SetFileForDescriptor(file, fd);
			return fd;
		}

		if(resolver.GetTotalElements() == 0)
		{
			task->FreeFileDescriptor(fd);
			return Error(KERN_INVALID_ARGUMENT, ENOENT);
		}

		if(flags & O_CREAT)
		{
			Instance *instance = parent->GetInstance();

			node = instance->CreateFile(context, parent, resolver.GetCurrentName()->GetCString());
			if(!node.IsValid())
			{	
				task->FreeFileDescriptor(fd);
				return node.GetError();
			}


			KernReturn<File *> file = instance->OpenFile(context, node, flags);
			if(!file.IsValid())
			{
				task->FreeFileDescriptor(fd);
				return file.GetError();
			}

			task->SetFileForDescriptor(file, fd);
			return fd;
		}

		task->FreeFileDescriptor(fd);

		return Error(KERN_RESOURCES_MISSING, ENOENT);
	}
	KernReturn<void> Close(Context *context, int fd)
	{
		OS::Task *task = context->GetTask();
		File *file = task->GetFileForDescriptor(fd);

		if(!file)
			return Error(KERN_INVALID_ARGUMENT, EBADF);

		Node *node = file->GetNode();
		Instance *instance = node->GetInstance();

		instance->CloseFile(context, file);
		task->FreeFileDescriptor(fd);

		return ErrorNone;
	}


	KernReturn<size_t> Write(Context *context, int fd, const void *data, size_t size)
	{
		OS::Task *task = context->GetTask();
		File *file = task->GetFileForDescriptor(fd);

		if(!file || !(file->GetFlags() & O_WRONLY || file->GetFlags() & O_RDWR))
			return Error(KERN_INVALID_ARGUMENT, EBADF);
		
		Node *node = file->GetNode();
		if(node->IsDirectory())
			return Error(KERN_INVALID_ARGUMENT, EISDIR);

		Instance *instance = node->GetInstance();
		return instance->FileWrite(context, file, data, size);
	}

	KernReturn<size_t> Read(Context *context, int fd, void *data, size_t size)
	{
		OS::Task *task = context->GetTask();
		File *file = task->GetFileForDescriptor(fd);

		if(!file || !(file->GetFlags() & O_RDONLY || file->GetFlags() & O_RDWR))
			return Error(KERN_INVALID_ARGUMENT, EBADF);
		
		Node *node = file->GetNode();
		if(node->IsDirectory())
			return Error(KERN_INVALID_ARGUMENT, EISDIR);

		Instance *instance = node->GetInstance();
		return instance->FileRead(context, file, data, size);
	}

	KernReturn<off_t> ReadDir(Context *context, int fd, dirent *entry, size_t count)
	{
		OS::Task *task = context->GetTask();
		File *file = task->GetFileForDescriptor(fd);

		if(!file || !(file->GetFlags() & O_RDONLY || file->GetFlags() & O_RDWR))
			return Error(KERN_INVALID_ARGUMENT, EBADF);
		
		Node *node = file->GetNode();
		Instance *instance = node->GetInstance();

		return instance->DirRead(context, entry, file, count);
	}

	KernReturn<off_t> Seek(Context *context, int fd, off_t offset, int whence)
	{
		OS::Task *task = context->GetTask();
		File *file = task->GetFileForDescriptor(fd);

		if(!file)
			return Error(KERN_INVALID_ARGUMENT, EBADF);

		Node *node = file->GetNode();
		Instance *instance = node->GetInstance();

		return instance->FileSeek(context, file, offset, whence);
	}

	KernReturn<void> StatFile(Context *context, const char *path, stat *buf)
	{
		Path resolver(path, context);

		while(resolver.GetElementsLeft() > 0)
		{
			KernReturn<Node *> result = resolver.ResolveElement();

			if(!result.IsValid())
				return result.GetError();
		}

		Node *node = resolver.GetCurrentNode();
		Instance *instance = node->GetInstance();

		struct stat temp;

		KernReturn<void> result = instance->FileStat(context, &temp, node);
		if(result.IsValid())
		{
			result = context->CopyDataIn(&temp, buf, sizeof(struct stat));
			return result;
		}

		return result;
	}


	KernReturn<void> Mount(Context *context, Instance *instance, const char *target)
	{
		Path resolver(target, context);

		while(resolver.GetElementsLeft() > 1)
		{
			KernReturn<Node *> result = resolver.ResolveElement();

			if(!result.IsValid())
				return result.GetError();
		}

		Node *node = resolver.GetCurrentNode();
		IO::StrongRef<IO::String> name = resolver.GetCurrentName();

		if(!node->IsDirectory())
			return Error(KERN_INVALID_ARGUMENT, ENOTDIR);

		Instance *nInstance = node->GetInstance();

		return nInstance->Mount(context, instance, static_cast<Directory *>(node), name->GetCString());
	}

	KernReturn<void> Unmount(Context *context, const char *path)
	{
		return ErrorNone;
	}


	void RegisterDescriptor(Descriptor *descriptor)
	{
		spinlock_lock(&_descriptorLock);
		_descriptors->AddObject(descriptor);
		spinlock_unlock(&_descriptorLock);
	}
	void UnregisterDescriptor(__unused Descriptor *descriptor)
	{}


	KernReturn<void> LoadInitrdModule(uint8_t *buffer, uint8_t *end)
	{
		Context *context = Context::GetKernelContext();

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
			KernReturn<int> fd = Open(context, name, O_WRONLY | O_CREAT);

			if(fd.IsValid())
			{
				size_t left = binaryLength;
				while(left > 0)
				{
					KernReturn<size_t> written = Write(context, fd, buffer, left);

					if(!written.IsValid())
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
		return ErrorNone;
	}
	
	KernReturn<void> Init()
	{
		_descriptors = IO::Array::Alloc()->Init();

		if(!_descriptors)
			return Error(KERN_NO_MEMORY);

		Descriptor *ffs = FFS::Descriptor::Alloc()->Init();

		if(!ffs)
		{
			kprintf("Failed to register FFS");
			return Error(KERN_NO_MEMORY);
		}
		// Create the root file system
		KernReturn<Instance *> instance;

		if((instance = ffs->CreateInstance()).IsValid() == false)
		{
			kprintf("Failed to create root instance");
			return instance.GetError();
		}

		_rootInstance = instance;
		_rootNode = _rootInstance->GetRootNode();

		Context::GetKernelContext(); // Initializes the kernel context and associates it with the kernel task

		// Create some basic folders
		KernReturn<void> result;

		if((result = MakeDirectory(Context::GetKernelContext(), "/usr")).IsValid() == false)
			return result;
		if((result = MakeDirectory(Context::GetKernelContext(), "/bin")).IsValid() == false)
			return result;
		if((result = MakeDirectory(Context::GetKernelContext(), "/etc")).IsValid() == false)
			return result;
		if((result = MakeDirectory(Context::GetKernelContext(), "/lib")).IsValid() == false)
			return result;


		bool didLoadInitrd = false;

		Sys::MultibootModule *module = Sys::bootInfo->modules;

		for(size_t i = 0; i < Sys::bootInfo->modulesCount; i ++)
		{
			if(strcmp((char *)module->name, "initrd") == 0)
			{
				uint8_t *buffer = reinterpret_cast<uint8_t *>(module->start);
				uint8_t *end    = reinterpret_cast<uint8_t *>(module->end);

				result = LoadInitrdModule(buffer, end);

				if(result.IsValid())
					didLoadInitrd = true;

				break;
			}

			module = module->GetNext();
		}

		if(!didLoadInitrd)
			kprintf("Couldn't find initrd");

		return ErrorNone;
	}
}
