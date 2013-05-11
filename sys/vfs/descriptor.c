//
//  descriptor.c
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
#include "descriptor.h"

vfs_descriptor_t *vfs_descriptorCreate(const char *name, uint32_t flags)
{
	assert(name && strlen(name) > 0);

	vfs_descriptor_t *descriptor = halloc(NULL, sizeof(vfs_descriptor_t));
	if(descriptor)
	{
		size_t length = strlen(name) + 1;

		descriptor->lock = SPINLOCK_INIT;
		descriptor->name = halloc(NULL, length);
		descriptor->instances = array_create();

		if(!descriptor->name || !descriptor->instances)
		{
			if(descriptor->name)
				hfree(NULL, descriptor->name);

			if(descriptor->instances)
				array_destroy(descriptor->instances);

			hfree(NULL, descriptor);
			return NULL;
		}

		strcpy(descriptor->name, name);

		descriptor->flags = flags;

		descriptor->createInstance  = NULL;
		descriptor->destroyInstance = NULL;

		descriptor->next = descriptor->prev = 0;
	}

	return descriptor;
}
