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
#include <video/video.h>
#include <kern/kprintf.h>
#include <kern/panic.h>
#include <kern/kern_return.h>
#include <libc/string.h>

#include <machine/cpu.h>
#include <machine/memory/memory.h>
#include <machine/interrupts/interrupts.h>
#include <machine/smp/smp.h>

const char *kVersionBeast    = "Nidhogg";
const char *kVersionAppendix = "";

BEGIN_EXTERNC
void sys_boot(multiboot_t *info) __attribute__ ((noreturn));
END_EXTERNC

extern void cxa_init();
extern kern_return_t pm_init(multiboot_t *info);

#define sys_init0(name, function) \
	do { \
		kprintf("Initializing %s... { ", name); \
		kern_return_t result; \
		if((result = function()) != KERN_SUCCESS) \
		{ \
			kprintf(" } failed\n"); \
			panic("Failed to initialize %s", name); \
		} \
		else \
		{ \
			kprintf(" } ok\n"); \
		} \
	} while(0)

#define sys_initN(name, function, ...) \
	do { \
		kprintf("Initializing %s... { ", name); \
		kern_return_t result; \
		if((result = function(__VA_ARGS__)) != KERN_SUCCESS) \
		{ \
			kprintf(" } failed\n"); \
			panic("Failed to initialize %s", name); \
		} \
		else \
		{ \
			kprintf(" } ok\n"); \
		} \
	} while(0)

void sys_boot(multiboot_t *info)
{
	// Get the C++ runtime and a basic video output ready
	cxa_init();
	vd::init();

	// Print the hello world message
	kprintf("Firedrake v%i.%i.%i:%i (%s)\nHere be dragons\n\n", kVersionMajor, kVersionMinor, kVersionPatch, VersionCurrent(), kVersionBeast);

	// Run the low level initialization process
	sys_init0("cpu", cpu_init);
	sys_initN("physical memory", pm::init, info);
	sys_initN("virtual memory", vm::init, info);
	sys_init0("heap", mm::heap_init);
	sys_init0("interrupts", ir::init);
	sys_init0("smp", smp_init);

	panic_init();
	kprintf("\n\n");

	while(1)
		cpu_halt();
}
