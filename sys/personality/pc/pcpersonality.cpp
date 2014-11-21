//
//  pcpersonality.cpp
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

#include "pcpersonality.h"

#if PERSONALITY == PERSONALITY_PC

#include <prefix.h>
#include <kern/kprintf.h>
#include <kern/panic.h>
#include <kern/kern_return.h>
#include <libc/string.h>
#include <libcpp/type_traits.h>

#include <machine/cpu.h>
#include <machine/port.h>
#include <machine/memory/memory.h>
#include <machine/interrupts/interrupts.h>
#include <machine/clock/clock.h>
#include <machine/smp/smp.h>
#include <kern/scheduler/scheduler.h>
#include <kern/syscall/syscall.h>
#include <vfs/vfs.h>

#define COM_PORT 0x3f8

namespace Sys
{
	void PrintUART(const char *string, size_t length)
	{
		for(size_t i = 0; i < length; i ++)
		{
			while((inb(COM_PORT + 5) & 0x20) == 0)
				CPUPause();
			 
			outb(COM_PORT, string[i]);
		}
	}

	void InitUART()
	{
		outb(COM_PORT + 1, 0x00);
		outb(COM_PORT + 3, 0x80);
		outb(COM_PORT + 0, 0x03);
		outb(COM_PORT + 1, 0x00);
		outb(COM_PORT + 3, 0x03);
		outb(COM_PORT + 2, 0xc7);
		outb(COM_PORT + 4, 0x0b);

		Sys::AddOutputHandler(&Sys::PrintUART);
	}

	



	PCPersonality::PCPersonality()
	{}

	const char *PCPersonality::GetName() const
	{
		return "PC";
	}


	void PCPersonality::FinishBootstrapping()
	{
		Init("syscalls", Core::SyscallInit);
		Init("vfs", VFS::Init);

		kprintf("\n\n");
	}

	void PCPersonality::InitSystem()
	{
		InitUART();
		
		// Print the hello world message
		PrintHeader();

		// Run the low level initialization process
		// Because stack is really limited, a lot of the work should be done in FinishBootstrapping() instead
		Init("cpu", Sys::CPUInit);
		Init("physical memory", Sys::PMInit);
		Init("virtual memory", Sys::VMInit);
		Init("heap", Sys::HeapInit);
		Init("interrupts", Sys::InterruptsInit);
		Init("clock", Sys::ClockInit);
		Init("smp", Sys::SMPInit);
		Init("scheduler", Core::SchedulerInit);

		// Kick off into the scheduler
		// Once the IPI is handled by a CPU, it won't return to its current thread of execution ever again
		// This means that whatever bootstrapping you want to do, MUST be done before this call
		APIC::BroadcastIPI(0x3a, true);
	}
}

#endif /* PERSONALITY == PERSONALITY_PC */
