//
//  boot.c
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

#include "multiboot.h"

#include <version.h>
#include <types.h>
#include <system/syslog.h>
#include <system/panic.h>
#include <system/video.h>

#include <system/state.h>
#include <system/interrupts.h>
#include <memory/memory.h>
#include <scheduler/scheduler.h>
#include <system/syscall.h>


typedef bool (*sys_function_t)(void *data);

void sys_init(char *name, sys_function_t function, void *data, bool essential)
{
	syslog(LOG_DEBUG, "Initializing %s... {", name);
	if(function(data))
	{
		syslog(LOG_DEBUG, "} ok\n");
	}
	else
	{
		syslog(LOG_DEBUG, "} failed\n");
		if(essential)
		{
			panic("Failed to initialize essential module \"%s\"!", name);
		}
	}
}


extern void _vd_init(); // Defined in system/video.c
extern void kerneld_main(); // Declared in kerneld/kerneld.c

void sys_boot(struct multiboot_t *info)
{	
	_vd_init();
	
#ifndef NDEBUG
	setLogLevel(LOG_DEBUG);
#else
	setLogLevel(LOG_INFO);
#endif
	
	syslog(LOG_INFO, "Firedrake v%i.%i.%i:%i%s (%s)\n", VersionMajor, VersionMinor, VersionPatch, VersionCurrent, versionAppendix, versionBeast);
	syslog(LOG_INFO, "Here be dragons!\n\n");

	// Color the dragon!
	vd_setColor(0, 0, true, vd_color_red, 9);

	// Load the modules
	sys_init("state", st_init, NULL, true); // Setup a global state
	sys_init("interrupts", ir_init, NULL, true); // REMARK! This doesn't enable interrupts! We must call ir_enableInterrupts() to enable them!
	sys_init("physical memory", pm_init, (void *)info, true);
	sys_init("virtual memory", vm_init, NULL, true); // After this point, never ever use unmapped memory again!
	sys_init("scheduler", sd_init, NULL, true);
	sys_init("syscall", sc_init, NULL, true);

	// Draw a boundary
	for(int i=0; i<80; i++)
		syslog(LOG_INFO, "-");

	syslog(LOG_DEBUG, "\n\n");

	// Prepare everything for the great travel...
	ir_enableInterrupts(); // Enable interrupts

	while(!sd_state()) // Wait until the scheduler is ready
		__asm__ volatile("hlt;");

	kerneld_main(); // Jump over to the kernel daemon which will do the rest of the work now
	panic("kerneld bugged"); // In the case that we leave kerneld, something really went wrong. PANIC!
}
