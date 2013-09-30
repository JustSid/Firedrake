//
//  cpu.cpp
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

#include <machine/interrupts/apic.h>
#include <kern/kprintf.h>
#include <libc/string.h>
#include "cpu.h"
#include "acpi.h"

#define CPU_MAX_CPUS 32

static cpu_info_t _cpu_info;

static cpu_t _cpu_cpu[CPU_MAX_CPUS];
static size_t _cpu_cpu_count = 0;
static cpu_t *_cpu_bootstrap_cpu = nullptr;

kern_return_t cpu_add_cpu(cpu_t *cpu)
{
	if(_cpu_cpu_count >= CPU_MAX_CPUS)
		return KERN_FAILURE;

	if((cpu->flags & CPU_FLAG_BOOTSTRAP) && _cpu_bootstrap_cpu)
	{
		kprintf("second bootstrap CPU");
		return KERN_FAILURE;
	}

	cpu_t *dest = &_cpu_cpu[_cpu_cpu_count];

	memcpy(dest, cpu, sizeof(cpu_t));
	dest->id = _cpu_cpu_count;

	if(dest->flags & CPU_FLAG_BOOTSTRAP)
		_cpu_bootstrap_cpu = dest;

	_cpu_cpu_count ++;
	return KERN_SUCCESS;
}

cpu_t *cpu_get_cpu_with_id(uint32_t id)
{
	if(id >= _cpu_cpu_count)
		return nullptr;

	return &_cpu_cpu[id];
}

cpu_t *cpu_get_cpu_with_apic(uint8_t apic)
{
	for(size_t i = 0; i < _cpu_cpu_count; i ++)
	{
		if(_cpu_cpu[i].apic_id == apic)
			return &_cpu_cpu[i];
	}

	return nullptr;
}

cpu_t *cpu_get_current_cpu()
{
	uint32_t id = ir::apic_read(APIC_REGISTER_ID);
	uint8_t apic = (id >> 24) & 0xff;

	return cpu_get_cpu_with_apic(apic);
}



void cpuid(cpuid_registers_t &registers)
{
	int *regs = reinterpret_cast<int *>(&registers);
	int code  = static_cast<int>(registers.eax);

	__asm__ volatile("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3]) : "a" (code), "c" (0));
}


cpu_info_t *cpu_get_info()
{
	return &_cpu_info;
}

void cpu_read_info()
{
	cpuid_registers_t registers = { 1, 0, 0, 0 };
	cpuid(registers);

	_cpu_info.stepping = (registers.eax >> 0) & 0xf;
	_cpu_info.model    = (registers.eax >> 4) & 0xf;
	_cpu_info.family   = (registers.eax >> 8) & 0xf;
	_cpu_info.type     = (registers.eax >> 12) & 0x3;

	_cpu_info.extendedModel  = (registers.eax >> 16) & 0xf;
	_cpu_info.extendedFamily = (registers.eax >> 20) & 0xf;

	_cpu_info.features = ((uint64_t)registers.edx << 32) | registers.ecx; 
}

kern_return_t cpu_init()
{
	cpu_read_info();

	// We need need an APIC and MSRs to run Firedrake
	if(!(_cpu_info.features & CPUID_FLAG_APIC) && !(_cpu_info.features & CPUID_FLAG_MSR))
	{
		kprintf("unsupported CPU (no MSR or APIC present)");
		return KERN_RESOURCES_MISSING;
	}

	kern_return_t result;
	if((result = acpi_init()) != KERN_SUCCESS)
		return result;

	if(!_cpu_bootstrap_cpu)
	{
		kprintf("no bootstrap CPU");
		return KERN_FAILURE;
	}

	return KERN_SUCCESS;
}
