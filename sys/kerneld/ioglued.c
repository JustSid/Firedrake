//
//  ioglued.c
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

#include <container/array.h>
#include <libc/stdio.h>
#include <libc/string.h>
#include <system/lock.h>
#include <system/syslog.h>
#include <system/helper.h>
#include <ioglue/iostore.h>
#include <ioglue/iomodule.h>
#include <scheduler/scheduler.h>
#include <vfs/vfs.h>

#include "ioglued.h"

static spinlock_t __ioglued_lock = SPINLOCK_INIT_LOCKED;
static array_t *__ioglued_modulesToStop = NULL;

void __ioglued_addReferencelessModule(io_module_t *module)
{
	spinlock_lock(&__ioglued_lock);

	array_addObject(__ioglued_modulesToStop, module);

	spinlock_unlock(&__ioglued_lock);
}

void ioglued()
{
	thread_setName(thread_getCurrentThread(), "ioglued", NULL);

	__ioglued_modulesToStop = array_create();
	spinlock_unlock(&__ioglued_lock);

	if(sys_checkCommandline("--no-ioglue", NULL))
	{
		while(1)
			sd_yield();
	}

	// Load all libraries marked in /etc/ioglue.conf
	int error;
	int fd = vfs_open(vfs_getKernelContext(), "/etc/ioglue.conf", O_RDONLY, &error);
	if(fd >= 0)
	{
		size_t size = vfs_seek(vfs_getKernelContext(), fd, 0, SEEK_END, &error);
		vfs_seek(vfs_getKernelContext(), fd, 0, SEEK_SET, &error);

		char *content = halloc(NULL, size + 1);
		char *temp = content;

		*(content + size) = '\0';

		while(size > 0)
		{
			size_t read = vfs_read(vfs_getKernelContext(), fd, temp, size, &error);
			size -= read;
			temp += read;
		}

		temp = content;

		while(temp)
		{
			char path[256];
			char *start = temp;

			temp = strpbrk(content, ",");
			if(temp)
				*temp = '\0';
			
			sprintf(path, "/lib/%s", start);

			io_module_t *module = io_moduleWithName(path);
			if(!module)
				dbg("ioglued: Failed to publish \"%s\"\n", path);

			temp = temp ? strstr(temp + 1, "lib") : temp;
		}

		hfree(NULL, content);
		vfs_close(vfs_getKernelContext(), fd);
	}
	else
	{
		warn("Couldn't open /etc/ioglue.conf, not loading any kernel modules!\n");
	}


	while(1) 
	{
		spinlock_lock(&__ioglued_lock);
		if(array_count(__ioglued_modulesToStop) > 0)
		{
			array_t *copy = array_copy(__ioglued_modulesToStop);
			array_removeAllObjects(__ioglued_modulesToStop);

			spinlock_unlock(&__ioglued_lock);

			for(size_t i=0; i<array_count(copy); i++)
			{
				io_module_t *module = array_objectAtIndex(copy, i);

				spinlock_lock(&module->lock);
				if(!module->initialized)
				{
					__ioglued_addReferencelessModule(module);
					spinlock_unlock(&module->lock);
					continue;
				}
				
				spinlock_unlock(&module->lock);
				io_moduleStop(module);
			}

			array_destroy(copy);
		}
		else
			spinlock_unlock(&__ioglued_lock);

		sd_yield();
	}
}
