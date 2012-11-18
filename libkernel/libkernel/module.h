//
//  module.h
//  libkernel
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

#ifndef _LIBKERNEL_MODULE_H_
#define _LIBKERNEL_MODULE_H_

#include "base.h"
#include "stdint.h"
#include "spinlock.h"

struct kern_module_s;

typedef bool (*kern_module_start_t)(struct kern_module_s *module);
typedef bool (*kern_module_stop_t)(struct kern_module_s *module);

typedef struct kern_module_s
{
	void *library;
	char *name;

	kern_spinlock_t lock;

	bool initialized;
	void *module;
	uint32_t references;

	kern_module_start_t start;
	kern_module_stop_t stop;
} kern_module_t;

kern_extern kern_module_t *kern_moduleWithName(const char *name);

kern_extern void kern_moduleRetain(kern_module_t *module);
kern_extern void kern_moduleRelease(kern_module_t *module);

#endif /* _LIBKERNEL_MODULE_H_ */
