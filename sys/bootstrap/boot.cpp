//
//  boot.cpp
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

#include "multiboot.h"

#include <prefix.h>
#include <video/video.h>
#include <kern/kprintf.h>
#include <kern/panic.h>
#include <kern/kern_return.h>
#include <libc/string.h>
#include <libcpp/type_traits.h>

#include <machine/cpu.h>
#include <machine/memory/memory.h>
#include <machine/interrupts/interrupts.h>
#include <machine/clock/clock.h>
#include <machine/smp/smp.h>
#include <kern/scheduler/scheduler.h>

const char *kVersionBeast    = "Nidhogg";
const char *kVersionAppendix = "";

extern "C" void sys_boot(multiboot_t *info) __attribute__ ((noreturn));

extern void cxa_init();

template<class F, class ...Args>
void sys_init(const char *name, F &&f, Args &&... args)
{
	kprintf("Initializing %s... { ", name);

	kern_return_t result;
	if((result = f(std::forward<Args>(args)...)) != KERN_SUCCESS)
	{
		kprintf("} failed\n");
		panic("Failed to initialize %s", name); 
	}
	else
	{
		kprintf(" } ok\n");
	}
}

void sys_boot(multiboot_t *info)
{
	// Get the C++ runtime and a basic video output ready
	cxa_init();
	vd::init();

	// Print the hello world message
	kprintf("Firedrake v%i.%i.%i:%i (%s)\nHere be dragons\n\n", kVersionMajor, kVersionMinor, kVersionPatch, VersionCurrent(), kVersionBeast);

	// Run the low level initialization process
	sys_init("cpu", cpu_init);
	sys_init("physical memory", pm::init, info);
	sys_init("virtual memory", vm::init, info);
	sys_init("heap", mm::heap_init);
	sys_init("interrupts", ir::init);
	sys_init("clock", clock::init);
	sys_init("smp", smp_init);
	sys_init("scheduler", sd::init);

	kprintf("\n\n");

	// Kick off into the scheduler
	// Once the IPI is handled by a CPU, it won't return to its current thread of execution ever again
	// This means that whatever bootstrapping you want to do, MUST be done before this call
	ir::apic_broadcast_ipi(0x3a, true);

	while(1)
		cpu_halt();
}
