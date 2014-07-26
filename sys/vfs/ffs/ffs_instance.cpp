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
	Instance::Instance()
	{
		VFS::Node *node;
		kern_return_t result = CreateDirectory(node, nullptr, nullptr, "");

		if(result == KERN_SUCCESS)
		{
			Initialize(node);
			node->Release();
		}
	}

	kern_return_t Instance::CreateFile(VFS::Node *&result, __unused VFS::Context *context, VFS::Node *parent, const char *name)
	{
		result = new FFS::Node(name, this, GetFreeID());

		if(result && parent)
		{
			VFS::Directory *directory = static_cast<VFS::Directory *>(parent);
			directory->Lock();
			kern_return_t res = directory->AttachNode(result);
			directory->Unlock();

			if(res != KERN_SUCCESS)
			{
				result->Release();
				return res;
			}
		}

		return result ? KERN_SUCCESS : KERN_NO_MEMORY;
	}

	kern_return_t Instance::CreateDirectory(VFS::Node *&result, __unused VFS::Context *context, VFS::Node *parent, const char *name)
	{
		result = new VFS::Directory(name, this, GetFreeID());

		if(result && parent)
		{
			VFS::Directory *directory = static_cast<VFS::Directory *>(parent);
			directory->Lock();
			kern_return_t res = directory->AttachNode(result);
			directory->Unlock();

			if(res != KERN_SUCCESS)
			{
				result->Release();
				return res;
			}
		}

		return result ? KERN_SUCCESS : KERN_NO_MEMORY;
	}
	kern_return_t Instance::DeleteNode(__unused VFS::Context *context, __unused VFS::Node *node)
	{
		return KERN_FAILURE;
	}



	kern_return_t Instance::LookUpNode(VFS::Node *&result, __unused VFS::Context *context, uint64_t id)
	{
		auto iterator = _nodes.find(static_cast<uint32_t>(id));
		if(iterator == _nodes.end())
		{
			result = nullptr;
			return KERN_RESOURCES_MISSING;
		}

		result = *iterator;
		return KERN_SUCCESS;
	}
	kern_return_t Instance::LookUpNode(VFS::Node *&result, __unused VFS::Context *context, VFS::Node *node, const char *name)
	{
		VFS::Directory *directory = static_cast<VFS::Directory *>(node);

		directory->Lock();
		result = directory->FindNode(name);
		directory->Unlock();

		return result ? KERN_SUCCESS : KERN_RESOURCES_MISSING;
	}


	kern_return_t Instance::OpenFile(VFS::File *&result, __unused VFS::Context *context, VFS::Node *node, int flags)
	{
		if(node->IsFile())
		{
			result = new VFS::File(node, flags);
			if(!result)
				return KERN_NO_MEMORY;

			if(flags & O_TRUNC)
				node->SetSize(0);

			return KERN_SUCCESS;
		}

		if(node->IsDirectory())
		{
			result = new VFS::FileDirectory(node, flags);
			if(!result)
				return KERN_NO_MEMORY;
			
			return KERN_SUCCESS;
		}

		return KERN_FAILURE;
	}

	void Instance::CloseFile(__unused VFS::Context *context, VFS::File *file)
	{
		delete file;
	}


	kern_return_t Instance::FileRead(size_t &read, VFS::Context *context, VFS::File *file, void *data, size_t size)
	{
		FFS::Node *node = static_cast<FFS::Node *>(file->GetNode());
		node->Lock();

		kern_return_t result = node->ReadData(context, file->GetOffset(), data, size, read);

		if(result == KERN_SUCCESS)
			file->SetOffset(file->GetOffset() + read);

		node->Unlock();

		return result;
	}
	kern_return_t Instance::FileWrite(size_t &written, VFS::Context *context, VFS::File *file, const void *data, size_t size)
	{
		FFS::Node *node = static_cast<FFS::Node *>(file->GetNode());
		node->Lock();

		kern_return_t result = node->WriteData(context, file->GetOffset(), data, size, written);

		if(result == KERN_SUCCESS)
			file->SetOffset(file->GetOffset() + written);

		node->Unlock();

		return result;
	}
	kern_return_t Instance::FileSeek(off_t &moved, __unused VFS::Context *context, VFS::File *file, off_t offset, int whence)
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
				return KERN_INVALID_ARGUMENT;
		}

		if(toffset <= node->GetSize())
		{
			file->SetOffset(toffset);
			moved = static_cast<off_t>(toffset);

			node->Unlock();
			return KERN_SUCCESS;
		}

		node->Unlock();
		return KERN_INVALID_ARGUMENT;
	}

	kern_return_t Instance::FileStat(stat *buf, __unused VFS::Context *context, VFS::Node *node)
	{	
		node->FillStat(buf);
		return KERN_SUCCESS;
	}

	kern_return_t Instance::DirRead(off_t &read, dirent *entry, VFS::Context *context, VFS::File *file, size_t count)
	{
		VFS::FileDirectory *directory = static_cast<VFS::FileDirectory *>(file);
		FFS::Node *node = static_cast<FFS::Node *>(file->GetNode());
		node->Lock();

		const struct dirent *entries = directory->GetEntries() + directory->GetOffset();

		size_t left = directory->GetCount() - directory->GetOffset();

		count = std::min(left, count);
		read  = 0;

		if(count > 0)
		{
			kern_return_t result = context->CopyDataIn(entries, entry, count * sizeof(struct dirent));
			if(result != KERN_SUCCESS)
			{
				node->Unlock();
				return result;
			}

			directory->SetOffset(directory->GetOffset() + count);
			read = count;
		}

		node->Unlock();

		return KERN_SUCCESS;
	}
}
