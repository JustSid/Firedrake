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

namespace FFS
{
	class Descriptor;
	class Instance : public VFS::Instance
	{
	public:
		Instance *Init();

		KernReturn<VFS::Node *> CreateFile(VFS::Context *context, VFS::Node *parent, const char *name) override;
		KernReturn<VFS::Node *> CreateDirectory(VFS::Context *context, VFS::Node *parent, const char *name) override;
		KernReturn<void> DeleteNode(VFS::Context *context, VFS::Node *node) override;

		KernReturn<IO::StrongRef<VFS::Node>> LookUpNode(VFS::Context *context, uint64_t id) override;
		KernReturn<IO::StrongRef<VFS::Node>> LookUpNode(VFS::Context *context, VFS::Node *node, const char *name) override;

		KernReturn<VFS::File *> OpenFile(VFS::Context *context, VFS::Node *node, int flags) override;
		void CloseFile(VFS::Context *context, VFS::File *file) override;
		KernReturn<void> FileStat(VFS::Context *context, stat *buf, VFS::Node *node) override;

		KernReturn<size_t> FileRead(VFS::Context *context, VFS::File *file, void *data, size_t size) override;
		KernReturn<size_t> FileWrite(VFS::Context *context, VFS::File *file, const void *data, size_t size) override;
		KernReturn<off_t> FileSeek(VFS::Context *context, VFS::File *file, off_t offset, int whence) override;
		KernReturn<off_t> DirRead(VFS::Context *context, dirent *entry, VFS::File *file, size_t count) override;

		KernReturn<void> Mount(VFS::Context *context, VFS::Instance *instance, VFS::Directory *target, const char *name) override;
		KernReturn<void> Unmount(VFS::Context *context, VFS::Mountpoint *target) override;

		KernReturn<void> Ioctl(VFS::Context *context, VFS::File *file, uint32_t request, void *data) override;
		
		IODeclareMeta(Instance)
	};
}

#endif /* _FFS_INSTANCE_H_ */
