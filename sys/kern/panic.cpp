//
//  panic.cpp
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

#include <machine/cpu.h>
#include <machine/interrupts/interrupts.h>
#include <libc/stdarg.h>
#include <libc/stdio.h>
#include <video/video.h>
#include "panic.h"
#include "kalloc.h"
#include "kprintf.h"

#define PANIC_STACK_SIZE 512
#define PANIC_HEAP_SIZE  2048

static bool _panic_initialized = false;

void panic_die_fancy(const char *buffer)
{
	cpu_t *cpu = cpu_get_current_cpu_slow();
	cpu_state_t *state = cpu->last_state;

	vd::video_device *device = vd::get_active_device();
	device->write_string("\n\16\24Kernel Panic!\16\27\n");
	device->write_string("Reason: \"");
	device->write_string(buffer);
	device->write_string("\"\n");

	// Use kprintf for small strings for convinience
	kprintf("Crashing CPU: \16\031%i\16\27\n", cpu->id);

	if(state)
	{
		kprintf("CPU State (interrupt vector \16\0310x%x\16\27):\n", state->interrupt);
		kprintf("  eax: \16\031%08x\16\27, ecx: \16\031%08x\16\27, edx: \16\031%08x\16\27, ebx: \16\031%08x\16\27\n", state->eax, state->ecx, state->edx, state->ebx);
		kprintf("  esp: \16\031%08x\16\27, ebp: \16\031%08x\16\27, esi: \16\031%08x\16\27, edi: \16\031%08x\16\27\n", state->esp, state->ebp, state->esi, state->edi);
		kprintf("  eip: \16\031%08x\16\27, eflags: \16\031%08x\16\27.\n", state->eip, state->eflags);
	}

	kprintf("CPU halt");
}

void panic_die_barebone(const char *buffer)
{
	vd::video_device *device = vd::get_active_device();
	device->write_string("\n\16\24Kernel Panic!\16\27\n");
	device->write_string("Reason: \"");
	device->write_string(buffer);
	device->write_string("\"\n\nCPU halt");
}

void panic_die(const char *buffer)
{
	if(!_panic_initialized)
	{
		panic_die_barebone(buffer);
		return;
	}

	panic_die_fancy(buffer);
}

void panic_stack(const char *reason, va_list args)
{
	char buffer[PANIC_STACK_SIZE];
	vsnprintf(buffer, PANIC_STACK_SIZE, reason, args);
	panic_die(buffer);
}

void panic_heap(const char *reason, va_list args)
{
	char *buffer = reinterpret_cast<char *>(kalloc(PANIC_HEAP_SIZE));
	if(!buffer)
	{
		panic_stack(reason, args);
		return;
	}

	vsnprintf(buffer, PANIC_HEAP_SIZE, reason, args);
	panic_die(buffer);
}

void panic(const char *reason, ...)
{
	if(_panic_initialized)
	{
		// TODO: SMP requires a different handling... A special IPI would probably be the way to halt the other CPUs
		cli();
	}

	va_list args;
	va_start(args, reason);

	_panic_initialized ? panic_heap(reason, args) : panic_stack(reason, args);

	va_end(args);

	while(1)
	{
		cli();
		cpu_halt();
	}
}

void panic_init()
{
	_panic_initialized = true;
}
