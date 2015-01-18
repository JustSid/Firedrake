//
//  node.h
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

#ifndef _VFS_NODE_H_
#define _VFS_NODE_H_

#include <prefix.h>
#include <libc/sys/spinlock.h>
#include <kern/kern_return.h>
#include <libc/stddef.h>
#include <libc/stdint.h>
#include <libc/sys/unistd.h>
#include <libc/sys/dirent.h>

#include <objects/IOObject.h>
#include <objects/IODictionary.h>
#include <objects/IOString.h>

namespace VFS
{
	class Instance;
	class Directory;

	class Node : public IO::Object
	{
	public:
		enum class Type
		{
			File,
			Directory,
			Link,
			Mountpoint
		};

		Node *Init(const char *name, Instance *instance, Type type, uint64_t id);
		void Dealloc() override;

		bool IsFile() const { return (_type == Type::File); }
		bool IsDirectory() const { return (_type == Type::Directory); }
		bool IsLink() const { return (_type == Type::Link); }
		bool IsMountpoint() const { return (_type == Type::Mountpoint); }

		uint64_t GetID() const { return _id; }
		uint64_t GetSize() const { return _size; }
		Type GetType() const { return _type; }
		IO::String *GetName() const { return _name; }
		Instance *GetInstance() const { return _instance; }
		Directory *GetParent() const { return _parent; }

		void Lock();
		void Unlock();

		// Lock must be held before calling these
		virtual void SetName(const char *name);
		virtual void SetSize(uint64_t size);

		virtual void FillStat(stat *buf);
		virtual void SetParent(Directory *parent);

	protected:
		spinlock_t _lock;

	private:
		IO::String *_name;
		uint64_t _id;
		uint64_t _size;
		Type _type;

		Directory *_parent;
		Instance *_instance;

		IODeclareMeta(Node)
	};

	class Directory : public Node
	{
	public:
		Directory *Init(const char *name, Instance *instance, uint64_t id);
		void Dealloc() override;

		// Lock must be held before calling these
		KernReturn<void> AttachNode(Node *node);
		KernReturn<void> RemoveNode(Node *node);
		KernReturn<void> RenameNode(Node *node, const char *filename); // Calls SetName() on the node!

		IO::StrongRef<Node> FindNode(const char *filename) const;
		const IO::Dictionary *GetChildren() const { return _children; }

	private:
		IO::Dictionary *_children;

		IODeclareMeta(Directory)
	};

	class Link : public Node
	{
	public:
		Link *Init(const char *name, Node *target, Instance *instance, uint64_t id);
		void Dealloc() override;

		IO::StrongRef<Node> GetTarget() const { return _target; }

	private:
		Node *_target;

		IODeclareMeta(Link)
	};

	class Mountpoint : public Node
	{
	public:
		Mountpoint *Init(const char *name, Instance *target, Instance *instance, uint64_t id);
		void Dealloc() override;

		void SetParent(Directory *parent) override;

		IO::StrongRef<Node> GetLinkedNode() const { return _linkedNode; }

	private:
		Instance *_target;
		Node *_linkedNode;

		IODeclareMeta(Mountpoint)
	};
}

#endif /* _VFS_NODE_H_ */
