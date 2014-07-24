//
//  instance.cpp
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

#include "instance.h"
#include "node.h"

namespace VFS
{
	Instance::Instance() :
		_lock(SPINLOCK_INIT),
		_lastID(0),
		_rootNode(nullptr)
	{}

	Instance::~Instance()
	{}

	void Instance::Lock()
	{
		spinlock_lock(&_lock);
	}
	void Instance::Unlock()
	{
		spinlock_unlock(&_lock);
	}

	void Instance::CleanNode(__unused Node *node)
	{}

	uint64_t Instance::GetFreeID()
	{
		return (_lastID ++);
	}

	void Instance::Initialize(Node *rootNode)
	{
		_rootNode = rootNode;
		_rootNode->Retain();
	}
}
