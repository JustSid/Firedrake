//
//  ffs_instance.h
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

#ifndef _FFS_INSTANCE_H_
#define _FFS_INSTANCE_H_

#include <vfs/instance.h>
#include <libcpp/hash_map.h>

namespace FFS
{
	class Descriptor;
	class Instance : public VFS::Instance
	{
	public:
		friend class Descriptor;

		kern_return_t CreateFile(VFS::Node *&result, VFS::Context *context, VFS::Node *parent, const char *name) override;
		kern_return_t CreateDirectory(VFS::Node *&result, VFS::Context *context, VFS::Node *parent, const char *name) override;
		kern_return_t DeleteNode(VFS::Context *context, VFS::Node *node) override;

		kern_return_t LookUpNode(VFS::Node *&result, VFS::Context *context, uint64_t id) override;
		kern_return_t LookUpNode(VFS::Node *&result, VFS::Context *context, VFS::Node *node, const char *name) override;

		kern_return_t OpenFile(VFS::File *&result, VFS::Context *context, VFS::Node *node, int flags) override;
		void CloseFile(VFS::Context *context, VFS::File *file) override;
		kern_return_t FileStat(VFS::Stat *stat, VFS::Context *context, VFS::Node *node) override;

		kern_return_t FileRead(size_t &result, VFS::Context *context, VFS::File *file, void *data, size_t size) override;
		kern_return_t FileWrite(size_t &result, VFS::Context *context, VFS::File *file, const void *data, size_t size) override;
		kern_return_t FileSeek(off_t &result, VFS::Context *context, VFS::File *file, off_t offset, int whence) override;
		kern_return_t DirRead(off_t &result, VFS::DirectoryEntry *entry, VFS::Context *context, VFS::File *file, size_t count) override;
		
	private:
		Instance();

		std::hash_map<uint32_t, VFS::Node *> _nodes;
	};
}

#endif /* _FFS_INSTANCE_H_ */
