//
//  node.cpp
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

#include <libc/string.h>
#include <kern/kprintf.h>

#include "node.h"
#include "instance.h"

namespace VFS
{
	IODefineMeta(Node, IO::Object)
	IODefineMeta(Directory, Node)
	IODefineMeta(Link, Node)
	IODefineMeta(Mountpoint, Node)

	Node *Node::Init(const char *name, Instance *instance, Type type, uint64_t id)
	{
		if(!Object::Init())
			return nullptr;

		spinlock_init(&_lock);

		_name = IO::String::Alloc()->InitWithCString(name);
		_instance = instance;
		_id = id;
		_type = type;
		_size = 0;
		_parent = nullptr;

		return this;
	}

	void Node::Dealloc()
	{
		_name->Release();
		Object::Dealloc();
	}
	


	void Node::Lock()
	{
		spinlock_lock(&_lock);
	}
	void Node::Unlock()
	{
		spinlock_unlock(&_lock);
	}


	void Node::SetName(const char *name)
	{
		SafeRelease(_name);
		_name = IO::String::Alloc()->InitWithCString(name);
	}
	void Node::SetSize(uint64_t size)
	{
		_size = size;
	}

	void Node::FillStat(stat *buf)
	{
		buf->type = static_cast<int>(_type);
		strlcpy(buf->name, _name->GetCString(), MAXNAME);

		buf->id   = _id;
		buf->size = _size;
	}

	void Node::SetParent(Directory *parent)
	{
		_parent = parent;
	}



	Directory *Directory::Init(const char *name, Instance *instance, uint64_t id)
	{
		if(!Node::Init(name, instance, Type::Directory, id))
			return nullptr;

		_children = IO::Dictionary::Alloc()->Init();

		return this;
	}

	void Directory::Dealloc()
	{
		_children->Release();
		Node::Dealloc();
	}

	KernReturn<void> Directory::AttachNode(Node *node)
	{
		if(!node || node->GetParent())
			return Error(KERN_INVALID_ARGUMENT);

		if(_children->GetObjectForKey(node->GetName()))
			return Error(KERN_RESOURCE_EXISTS);

		_children->SetObjectForKey(node, node->GetName());
		node->SetParent(this);

		return ErrorNone;
	}
	KernReturn<void> Directory::RemoveNode(Node *node)
	{
		if(!node || node->GetParent() != this)
			return Error(KERN_INVALID_ARGUMENT);

		if(_children->GetObjectForKey(node->GetName()) != node)
			return Error(KERN_INVALID_ARGUMENT); 

		_children->RemoveObjectForKey(node->GetName());
		node->SetParent(nullptr);

		return ErrorNone;
	}
	KernReturn<void> Directory::RenameNode(Node *node, const char *filename)
	{
		if(node->GetParent() != this)
			return Error(KERN_INVALID_ARGUMENT);

		Node *result = _children->GetObjectForKey<Node>(node->GetName());
		if(result != node)
			return Error(KERN_INVALID_ARGUMENT);

		IO::String *temp = IO::String::Alloc()->InitWithCString(filename);
		result = _children->GetObjectForKey<Node>(temp);
		temp->Release();

		if(result)
			return Error(KERN_RESOURCE_EXISTS);


		// Actually rename and update the child
		node->Retain();

		_children->RemoveObjectForKey(node->GetName());
		node->SetName(filename);
		_children->SetObjectForKey(node, node->GetName());

		node->Release();

		return ErrorNone;
	}
	IO::StrongRef<Node> Directory::FindNode(const char *filename) const
	{
		IO::String *temp = IO::String::Alloc()->InitWithCString(filename);
		IO::StrongRef<Node> node = _children->GetObjectForKey<Node>(temp);
		temp->Release();

		return node;
	}


	Link *Link::Init(const char *name, Node *target, Instance *instance, uint64_t id)
	{
		if(!Node::Init(name, instance, Type::Link, id))
			return nullptr;

		_target = target->Retain();

		return this;
	}

	void Link::Dealloc()
	{
		_target->Release();
		Node::Dealloc();
	}


	Mountpoint *Mountpoint::Init(const char *name, Instance *target, Instance *instance, uint64_t id)
	{
		if(!Node::Init(name, instance, Type::Mountpoint, id))
			return nullptr;

		_target = target->Retain();
		_linkedNode = _target->GetRootNode();

		_target->SetMountpoint(this);

		return this;
	}

	void Mountpoint::Dealloc()
	{
		_target->Release();
		Node::Dealloc();
	}

	void Mountpoint::SetParent(Directory *parent)
	{
		Node::SetParent(parent);
		_linkedNode->SetParent(parent);
	}
}
