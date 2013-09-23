//
//  boot.cpp
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

#include "multiboot.h"

#include <prefix.h>
#include <kern/kprintf.h>
#include <kern/kern_return.h>

const char *kVersionBeast    = "Nidhogg";
const char *kVersionAppendix = "";

BEGIN_EXTERNC
void sys_boot(multiboot_t *info) __attribute__ ((noreturn));
END_EXTERNC

extern void cxa_init();
extern void vd_init();
extern kern_return_t pm_init(multiboot_t *info);

#define sys_init(name, function, data) \
	do { \
		kprintf("Initializing %s... {", name); \
		kern_return_t result; \
		if((result = function(data)) != KERN_SUCCESS) \
		{ \
			kprintf("} failed"); /* This should probably be handled better ;) */ \
		} \
		else \
		{ \
			kprintf("} ok\n"); \
		} \
	} while(0)

void sys_boot(multiboot_t *info)
{
	// Get the C++ runtime and a basic video output ready
	cxa_init();
	vd_init();

	// Print the hello world message
	kprintf("Firedrake v%i.%i.%i:%i (%s)\nHere be dragons\n\n", kVersionMajor, kVersionMinor, kVersionPatch, VersionCurrent(), kVersionBeast);

	// Run the low level initialization process
	sys_init("physical memory", pm_init, info);

	while(1) {}
}
