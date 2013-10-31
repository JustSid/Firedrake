//
//  smp.cpp
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

#include <kern/kprintf.h>
#include <kern/spinlock.h>
#include <libc/string.h>
#include <machine/port.h>
#include <machine/cme.h>
#include "smp.h"

extern "C" void smp_rendezvous_point();
extern "C" uintptr_t smp_bootstrap_begin;
extern "C" uintptr_t smp_bootstrap_end;

static spinlock_t _smp_rendezvous_lock = SPINLOCK_INIT;
static size_t _smp_ticks = 0;

void smp_rendezvous_point()
{
	// TODO: Implement an actual function here
	kprintf("Hello World\n");
	while(1) {}
}

void smp_clock_tick(__unused uint8_t vector, __unused cpu_t *cpu)
{
	_smp_ticks ++;
}

void smp_kickoff_cpu(cpu_t *cpu, uint32_t entry)
{
	uint32_t vector = APIC_ICR_DM_INIT | APIC_ICR_LV_ASSERT;
	ir::apic_send_ipi(0x0f, cpu, vector);

	// Wait 10ms for the CPU to initialize
	size_t ticksEnd = _smp_ticks + 10;
	while(_smp_ticks <= ticksEnd)
		cpu_halt();

	vector = ((entry / 0x1000) & 0xff) | APIC_ICR_DSS_OTHERS | APIC_ICR_DM_STARTUP | APIC_ICR_LV_ASSERT;
	ir::apic_send_ipi(0x0, cpu, vector);

	// Wait for the rendezvous
	bool rendezvoused = false;

	ticksEnd = _smp_ticks + 100;
	while(_smp_ticks <= ticksEnd)
	{
		cpu_halt();

		if(cpu->flags & CPU_FLAG_RUNNING)
		{
			rendezvoused = true;
			break;
		}
	}

	if(!rendezvoused)
		kprintf("CPU didn't start\n");
}

kern_return_t smp_init()
{	
	return KERN_SUCCESS;

	uint8_t *bootstrapBegin = reinterpret_cast<uint8_t *>(&smp_bootstrap_begin);
	uint8_t *bootstrapEnd   = reinterpret_cast<uint8_t *>(&smp_bootstrap_end);

	size_t pages = VM_PAGE_COUNT(bootstrapEnd - bootstrapBegin);

	kern_return_t result;
	uintptr_t paddress;
	vm_address_t vaddress;

	if((result = pm::alloc_limit(paddress, pages, VM_LOWER_LIMIT, 0xff000)) != KERN_SUCCESS)
	{
		kprintf("failed to allocate physical memory");
		return result;
	}

	if((result = vm::get_kernel_directory()->alloc(vaddress, paddress, pages, VM_FLAGS_KERNEL)) != KERN_SUCCESS)
	{
		kprintf("failed to allocate virtual memory");
		return result;
	}

	uint8_t *buffer = reinterpret_cast<uint8_t *>(vaddress);
	memcpy(buffer, bootstrapBegin, bootstrapEnd - bootstrapBegin);

	cme_fix_call_simple(buffer, bootstrapEnd - bootstrapBegin, reinterpret_cast<void *>(&smp_rendezvous_point));

	// Set up the timer
	int divisor = 119318;
	outb(0x43, 0x36);
	outb(0x40, divisor & 0xff);
	outb(0x40, divisor >> 8);

	ir::set_interrupt_handler(0x20, &smp_clock_tick);

	// Start up all CPUs
	size_t count = cpu_get_cpu_count();
	for(size_t i = 0; i < count; i ++)
	{
		cpu_t *cpu = cpu_get_cpu_with_id(i);
		if(cpu->flags & CPU_FLAG_BOOTSTRAP)
			continue;

		smp_kickoff_cpu(cpu, paddress);
	}

	return KERN_SUCCESS;
}
