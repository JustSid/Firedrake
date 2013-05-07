//
//  cpu.c
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

#include <libc/string.h>
#include <memory/memory.h>
#include "syslog.h"
#include "cpu.h"

static cpu_info_t _cpu_info;

void cpuid(struct cpuid_registers_s *registers)
{
	int *regs = (int *)registers;
	int code = registers->eax;

	__asm__ volatile("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3]) : "a" (code), "c" (0));
}

void cpu_readVendor()
{
	struct cpuid_registers_s registers = { 0, 0, 0, 0 };
	cpuid(&registers);

	char vendor[13] = { 0 };
	memcpy(vendor + 0, &registers.ebx, 4);
	memcpy(vendor + 4, &registers.edx, 4);
	memcpy(vendor + 8, &registers.ecx, 4);

	_cpu_info.vendor = cpu_vendor_Unknown;

	if(strcmp(vendor, "AuthenticAMD") == 0)
	{
		_cpu_info.vendor = cpu_vendor_AMD;
	}
	if(strcmp(vendor, "GenuineIntel") == 0)
	{
		_cpu_info.vendor = cpu_vendor_Intel;
	}
}

void cpu_readFeatures()
{
	struct cpuid_registers_s registers = { 1, 0, 0, 0 };
	cpuid(&registers);

	_cpu_info.stepping = (registers.eax >> 0) & 0xF;
	_cpu_info.model    = (registers.eax >> 4) & 0xF;
	_cpu_info.family   = (registers.eax >> 8) & 0xF;
	_cpu_info.type     = (registers.eax >> 12) & 0x3;

	_cpu_info.extendedModel  = (registers.eax >> 16) & 0xF;
	_cpu_info.extendedFamily = (registers.eax >> 20) & 0xF;

	_cpu_info.features = ((uint64_t)registers.edx << 32) | registers.ecx; 
}

bool cpu_init(__unused void *data)
{
	cpu_readVendor();
	cpu_readFeatures();

	if(_cpu_info.features & kCPUFeatureAPIC && !(_cpu_info.features & kCPUFeatureMSR))
	{
		info("CPU has APIC support but no MSRs!");
		return false;
	}

	// Dump some info about the CPU
	switch(_cpu_info.vendor)
	{
		case cpu_vendor_AMD:
			info("AMD ");
			break;

		case cpu_vendor_Intel:
			info("Intel ");
			break;

		default:
			info("Unknown CPU vendor");
			return false; // Picky kernel is picky
	}

	info("%hhi %hhi %hhi.", _cpu_info.family, _cpu_info.model, _cpu_info.stepping);

#define CheckAndDumpCPUFeature(feature) \
	do { \
		if(_cpu_info.features & kCPUFeature##feature) \
		{ \
			dbg(" %s", #feature); \
		} \
	} while(0)

	CheckAndDumpCPUFeature(APIC);
	CheckAndDumpCPUFeature(MSR);

	CheckAndDumpCPUFeature(SSE);
	CheckAndDumpCPUFeature(SSE2);

	CheckAndDumpCPUFeature(SEP);

#undef CheckAndDumpCPUFeature

	return true;
}
