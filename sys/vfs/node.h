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
#include <kern/spinlock.h>
#include <kern/kern_return.h>
#include <libc/stddef.h>
#include <libc/stdint.h>
#include <libc/sys/unistd.h>
#include <libc/sys/dirent.h>
#include <libcpp/atomic.h>
#include <libcpp/string.h>
#include <libcpp/hash_map.h>

namespace VFS
{
	using Filename = std::fixed_string<MAXNAME>;

	class Instance;
	class Directory;

	class Node
	{
	public:
		friend class Directory;

		enum class Type
		{
			File,
			Directory,
			Link
		};

		virtual ~Node();

		bool IsFile() const { return (_type == Type::File); }
		bool IsDirectory() const { return (_type == Type::Directory); }
		bool IsLink() const { return (_type == Type::Link); }

		uint64_t GetID() const { return _id; }
		uint64_t GetSize() const { return _size; }
		Type GetType() const { return _type; }
		const Filename &GetName() const { return _name; }
		Instance *GetInstance() const { return _instance; }
		Directory *GetDirectory() const { return _parent; }

		void Retain();
		void Release();

		void Lock();
		void Unlock();

		// Lock must be held before calling these
		void SetName(const Filename &name);
		void SetSize(uint64_t size);
		void FillStat(stat *buf);

	protected:
		Node(const char *name, Instance *instance, Type type, uint64_t id);
		spinlock_t _lock;

	private:
		Filename _name;
		uint64_t _id;
		uint64_t _size;
		Type _type;

		Directory *_parent;
		Instance *_instance;
		std::atomic<uint32_t> _references;
	};
}

namespace std
{
	template<>
	struct hash<VFS::Filename>
	{
		size_t operator() (const VFS::Filename &name) const
		{
			size_t hash   = 0;
			size_t length = name.length();

			const char *string = name.c_str();

			for(size_t i = 0; i < length; i ++)
				hash_combine(hash, static_cast<int32_t>(string[i]));

			return hash;
		}
	};

	template<>
	struct equal_to<VFS::Filename>
	{
	public:
		bool operator() (const VFS::Filename &a, const VFS::Filename &b) const
		{
			return (a == b);
		}
	};
}

namespace VFS
{
	class Directory : public Node
	{
	public:
		Directory(const char *name, Instance *instance, uint64_t id);
		~Directory() override;

		// Lock must be held before calling these
		kern_return_t AttachNode(Node *node);
		kern_return_t RemoveNode(Node *node);
		kern_return_t RenameNode(Node *node, const Filename &name); // Calls SetName() on the node!

		Node *FindNode(const Filename &name) const;

		std::hash_map<Filename, Node *>::iterator GetBegin() { return _children.begin(); }
		std::hash_map<Filename, Node *>::iterator GetEnd() { return _children.end(); }

	private:
		std::hash_map<Filename, Node *> _children;
	};
}

#endif /* _VFS_NODE_H_ */
