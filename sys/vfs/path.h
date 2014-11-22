//
//  path.h
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

#ifndef _VFS_PATH_H_
#define _VFS_PATH_H_

#include <prefix.h>
#include <kern/kern_return.h>
#include "node.h"
#include "context.h"

namespace VFS
{
	class Path
	{
	public:
		Path(const char *path, Context *context);
		~Path();

		Node *GetCurrentNode() const { return _node; }
		const Filename &GetCurrentName() const { return _name; }
		Filename GetNextName();

		size_t GetElementsLeft() const { return _elementsLeft; }
		size_t GetTotalElements() const { return _totalElements; }

		KernReturn<Node *> ResolveElement();

	private:
		char *_path;
		char *_element;

		Filename _name;

		Context *_context;
		Node *_node;

		size_t _totalElements;
		size_t _elementsLeft;
	};
}

#endif
