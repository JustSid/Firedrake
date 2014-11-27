//
//  ffs_instance.cpp
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

#include <kern/kprintf.h>
#include <vfs/file.h>
#include <vfs/context.h>
#include "ffs_instance.h"
#include "ffs_node.h"

namespace FFS
{
	IODefineMeta(Instance, VFS::Instance)

	Instance *Instance::Init()
	{
		KernReturn<VFS::Node *> node = CreateDirectory(nullptr, nullptr, "");

		if(!node.IsValid())
			return nullptr;

		if(!VFS::Instance::Init(node))
			return nullptr;

		node->Release();
		return this;
	}

	KernReturn<VFS::Node *> Instance::CreateFile(__unused VFS::Context *context, VFS::Node *parent, const char *name)
	{
		VFS::Node *node = FFS::Node::Alloc()->Init(name, this, GetFreeID());

		if(node && parent)
		{
			VFS::Directory *directory = static_cast<VFS::Directory *>(parent);

			directory->Lock();
			KernReturn<void> result = directory->AttachNode(node);
			directory->Unlock();

			if(!result.IsValid())
			{
				node->Release();
				return result.GetError();
			}
		}

		if(!node)
			return Error(KERN_NO_MEMORY);

		return node;
	}

	KernReturn<VFS::Node *> Instance::CreateDirectory(__unused VFS::Context *context, VFS::Node *parent, const char *name)
	{
		VFS::Node *node = VFS::Directory::Alloc()->Init(name, this, GetFreeID());

		if(node && parent)
		{
			VFS::Directory *directory = static_cast<VFS::Directory *>(parent);
			directory->Lock();
			KernReturn<void> result = directory->AttachNode(node);
			directory->Unlock();

			if(!result.IsValid())
			{
				node->Release();
				return result.GetError();
			}
		}

		if(!node)
			return Error(KERN_NO_MEMORY);

		return node;
	}
	KernReturn<void> Instance::DeleteNode(__unused VFS::Context *context, __unused VFS::Node *node)
	{
		return Error(KERN_FAILURE);
	}



	KernReturn<IO::StrongRef<VFS::Node>> Instance::LookUpNode(__unused VFS::Context *context, __unused uint64_t id)
	{
		return Error(KERN_RESOURCES_MISSING);
	}
	KernReturn<IO::StrongRef<VFS::Node>> Instance::LookUpNode(__unused VFS::Context *context, VFS::Node *tnode, const char *name)
	{
		VFS::Directory *directory = static_cast<VFS::Directory *>(tnode);

		directory->Lock();
		IO::StrongRef<VFS::Node> node = directory->FindNode(name);
		directory->Unlock();

		if(!node)
			return Error(KERN_RESOURCES_MISSING);

		return node;
	}


	KernReturn<VFS::File *> Instance::OpenFile(__unused VFS::Context *context, VFS::Node *node, int flags)
	{
		if(node->IsFile())
		{
			VFS::File *file = VFS::File::Alloc()->Init(node, flags);
			if(!file)
				return Error(KERN_NO_MEMORY);

			if(flags & O_TRUNC)
				node->SetSize(0);

			return file;
		}

		if(node->IsDirectory())
		{
			VFS::File *file = VFS::FileDirectory::Alloc()->Init(node, flags);
			if(!file)
				return Error(KERN_NO_MEMORY);
			
			return file;
		}

		return Error(KERN_FAILURE);
	}

	void Instance::CloseFile(__unused VFS::Context *context, VFS::File *file)
	{
		file->Release();
	}


	KernReturn<size_t> Instance::FileRead(VFS::Context *context, VFS::File *file, void *data, size_t size)
	{
		FFS::Node *node = static_cast<FFS::Node *>(file->GetNode());
		node->Lock();

		KernReturn<size_t> result = node->ReadData(context, file->GetOffset(), data, size);

		if(result.IsValid())
			file->SetOffset(file->GetOffset() + result.Get());

		node->Unlock();

		return result;
	}
	KernReturn<size_t> Instance::FileWrite(VFS::Context *context, VFS::File *file, const void *data, size_t size)
	{
		FFS::Node *node = static_cast<FFS::Node *>(file->GetNode());
		node->Lock();

		KernReturn<size_t> result = node->WriteData(context, file->GetOffset(), data, size);

		if(result.IsValid())
			file->SetOffset(file->GetOffset() + result.Get());

		node->Unlock();

		return result;
	}
	KernReturn<off_t> Instance::FileSeek(__unused VFS::Context *context, VFS::File *file, off_t offset, int whence)
	{
		FFS::Node *node = static_cast<FFS::Node *>(file->GetNode());
		node->Lock();

		size_t toffset = 0;

		switch(whence)
		{
			case SEEK_SET:
				toffset = offset;
				break;
			case SEEK_CUR:
				toffset = file->GetOffset() + offset;
				break;
			case SEEK_END:
				toffset = node->GetSize() + offset;
				break;

			default:
				node->Unlock();
				return Error(KERN_INVALID_ARGUMENT);
		}

		if(toffset <= node->GetSize())
		{
			file->SetOffset(toffset);
			node->Unlock();
			
			return static_cast<off_t>(toffset);
		}

		node->Unlock();
		return Error(KERN_INVALID_ARGUMENT);
	}

	KernReturn<void> Instance::FileStat(stat *buf, __unused VFS::Context *context, VFS::Node *node)
	{	
		node->FillStat(buf);
		return ErrorNone;
	}

	KernReturn<off_t> Instance::DirRead(dirent *entry, VFS::Context *context, VFS::File *file, size_t count)
	{
		VFS::FileDirectory *directory = static_cast<VFS::FileDirectory *>(file);
		FFS::Node *node = static_cast<FFS::Node *>(file->GetNode());
		node->Lock();

		const struct dirent *entries = directory->GetEntries() + directory->GetOffset();
		size_t left = directory->GetCount() - directory->GetOffset();

		count = std::min(left, count);
		size_t read  = 0;

		if(count > 0)
		{
			KernReturn<void> result = context->CopyDataIn(entries, entry, count * sizeof(struct dirent));
			if(!result.IsValid())
			{
				node->Unlock();
				return result.GetError();
			}

			directory->SetOffset(directory->GetOffset() + count);
			read = count;
		}

		node->Unlock();
		return read;
	}
}
