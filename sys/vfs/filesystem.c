//
//  filesystem.c
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2013 by Sidney Just
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

#include <memory/memory.h>
#include <libc/string.h>
#include "filesystem.h"
#include "context.h"

vfs_instance_t *vfs_instanceCreate(vfs_descriptor_t *descriptor, vfs_callbacks_t *callbacks, void *data)
{
	assert(descriptor);
	assert(callbacks);

	spinlock_lock(&descriptor->lock);

	vfs_instance_t *instance = halloc(NULL, sizeof(vfs_instance_t));
	if(instance)
	{
		instance->lock = SPINLOCK_INIT;
		instance->descriptor = descriptor;
		instance->lastID = 0;

		instance->data = data;

		memcpy(&instance->callbacks, callbacks, sizeof(vfs_callbacks_t));

		// Create the root node and mount point
		int errno;

		instance->root = callbacks->createDirectory(instance, vfs_getKernelContext(), NULL, "", &errno);
		instance->mountPoint = callbacks->createLink(instance, vfs_getKernelContext(), NULL, instance->root, "", &errno);

		array_addObject(descriptor->instances, instance);
	}

	spinlock_unlock(&descriptor->lock);

	return instance;
}
