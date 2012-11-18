//
//  ioglued.c
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2012 by Sidney Just
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
#include <system/lock.h>
#include <system/syslog.h>
#include <ioglue/iostore.h>
#include <ioglue/iomodule.h>
#include <scheduler/scheduler.h>

#include "ioglued.h"

static spinlock_t __ioglued_lock = SPINLOCK_INIT_LOCKED;
static array_t *__ioglued_modulesToStop = NULL;

const char *ioglued_modules[] = {
	"libtest.so"
};

void __ioglued_addReferencelessModule(io_module_t *module)
{
	spinlock_lock(&__ioglued_lock);

	array_addObject(__ioglued_modulesToStop, module);

	spinlock_unlock(&__ioglued_lock);
}

void ioglued()
{
	thread_setName(thread_getCurrentThread(), "syslogd");

	__ioglued_modulesToStop = array_create();
	spinlock_unlock(&__ioglued_lock);

	size_t size = sizeof(ioglued_modules) / sizeof(const char *);
	for(size_t i=0; i<size; i++)
	{
		const char *name = ioglued_modules[i];
		io_module_t *module = io_moduleWithName(name);

		if(!module)
			dbg("ioglued: Failed to publish \"%s\"\n", name);
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
				io_moduleStop(module);
			}

			array_destroy(copy);
		}
		else
			spinlock_unlock(&__ioglued_lock);

		sd_yield();
	}
}
