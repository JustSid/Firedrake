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
	Node::Node(const char *name, Instance *instance, Type type, uint64_t id) :
		_instance(instance),
		_type(type),
		_id(id),
		_size(0),
		_lock(SPINLOCK_INIT),
		_references(1),
		_parent(nullptr)
	{
		SetName(name);
	}

	Node::~Node()
	{}

	void Node::Retain()
	{
		_references ++;
	}
	void Node::Release()
	{
		if((-- _references) == 0)
		{
			_instance->CleanNode(this);
			delete this;
		}
	}

	void Node::Lock()
	{
		spinlock_lock(&_lock);
	}
	void Node::Unlock()
	{
		spinlock_unlock(&_lock);
	}


	void Node::SetName(const Filename &name)
	{
		_name = name;
	}
	void Node::SetSize(uint64_t size)
	{
		_size = size;
	}

	void Node::FillStat(Stat *stat)
	{
		stat->type = static_cast<int>(_type);
		strcpy(stat->name, _name.c_str());

		stat->id = _id;
		stat->size = _size;
	}



	Directory::Directory(const char *name, Instance *instance, uint64_t id) :
		Node(name, instance, Type::Directory, id)
	{}

	Directory::~Directory()
	{
		for(auto iterator = _children.begin(); iterator != _children.end(); iterator ++)
		{
			Node *node = *iterator;
			node->Release();
		}
	}

	kern_return_t Directory::AttachNode(Node *node)
	{
		if(node->_parent)
			return KERN_INVALID_ARGUMENT;

		if(_children.find(node->GetName()) != _children.end())
			return KERN_RESOURCE_EXISTS;

		_children.insert(node->GetName(), node);
		node->Retain();
		
		return KERN_SUCCESS;
	}
	kern_return_t Directory::RemoveNode(Node *node)
	{
		if(node->_parent != this)
			return KERN_INVALID_ARGUMENT;

		auto iterator = _children.find(node->GetName());

		if(iterator == _children.end())
			return KERN_INVALID_ARGUMENT;

		_children.erase(iterator);
		node->Release();

		return KERN_SUCCESS;
	}
	kern_return_t Directory::RenameNode(Node *node, const Filename &name)
	{
		if(node->_parent != this)
			return KERN_INVALID_ARGUMENT;

		auto iterator = _children.find(node->GetName());

		if(iterator == _children.end())
			return KERN_INVALID_ARGUMENT;

		_children.erase(iterator);
		_children.insert(name, node);

		node->SetName(name);
		return KERN_SUCCESS;
	}
	Node *Directory::FindNode(const Filename &name) const
	{
		auto iterator = _children.find(name);
		Node *result = (iterator != _children.end()) ? *iterator : nullptr;

		if(result)
			result->Retain();

		return result;
	}
}