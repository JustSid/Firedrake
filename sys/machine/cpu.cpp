//
//  cpu.cpp
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

#include <machine/interrupts/apic.h>
#include <machine/memory/memory.h>
#include <kern/kprintf.h>
#include <kern/panic.h>
#include <libc/string.h>
#include <libcpp/new.h>
#include "cpu.h"
#include "acpi.h"
#include "gdt.h"

namespace Sys
{
	// Small tiny race condition...
	// CPUs are initialized _before_ the memory is, so we don't really have memory for this...
	// Get some scratch memory and use that instead
	char _cpuMemory[CONFIG_MAX_CPUS * sizeof(CPU)];

	static CPU *_cpu = reinterpret_cast<CPU *>(_cpuMemory);
	static CPU *_bootstrapCPU;
	static size_t _cpuCount = 0;

	static bool _hasCPUFastPath = false;

	// -----
	// CPU
	// -----

	CPU::CPU(uint8_t id, uint8_t apicID, Flags flags) :
		_id(id),
		_apicID(apicID),
		_flags(flags),
		_lastState(nullptr),
		_trampoline(nullptr)
	{}


	CPU *CPU::RegisterCPU(uint8_t apicID, Flags flags)
	{
		if(_cpuCount >= CONFIG_MAX_CPUS)
		{
			kprintf("Trying to register more CPUs than configured (increase CONFIG_MAX_CPUS)");
			return nullptr;
		}

		if((flags & Flags::Bootstrap) && _bootstrapCPU)
			panic("Tried to register more than one bootstrap CPU!");

		uint8_t id = _cpuCount ++;
		CPU *cpu = new(&_cpuMemory[id * sizeof(CPU)]) CPU(id, apicID, flags);

		if(flags & Flags::Bootstrap)
			_bootstrapCPU = cpu;

		return cpu;
	}

	void CPU::Bootstrap()
	{
		_info = CPUInfo();
	}


	void CPU::SetState(CPUState *state)
	{
		_lastState = state;
	}
	void CPU::SetTrampoline(Trampoline *trampoline)
	{
		_trampoline = trampoline;
	}
	void CPU::SetFlags(Flags flags)
	{
		_flags = flags;
	}


	CPU *CPU::GetCPUWithID(uint32_t id)
	{
		if(id >= _cpuCount)
			return nullptr;

		return &_cpu[id];
	}
	CPU *CPU::GetCPUWithApicID(uint32_t apicID)
	{
		for(size_t i = 0; i < _cpuCount; i ++)
		{
			if(_cpu[i]._apicID == apicID)
				return &_cpu[i];
		}

		return nullptr;
	}
	CPU *CPU::GetCurrentCPU()
	{
		if(!_hasCPUFastPath)
		{
			uint32_t id  = APIC::Read(APIC::Register::ID);
			uint8_t apic = (id >> 24) & 0xff;

			return GetCPUWithApicID(apic);
		}

		uint16_t id;
		__asm__ volatile("movw %%fs, %0" : "=a" (id));

		return &_cpu[id];
	}

	uint32_t CPU::GetCPUID()
	{
		if(!_hasCPUFastPath)
		{
			uint32_t id  = APIC::Read(APIC::Register::ID);
			uint8_t apic = (id >> 24) & 0xff;

			CPU *cpu = GetCPUWithApicID(apic);
			if(!cpu)
				panic("Running on unregistered and unknown CPU!");

			return cpu->_id;
		}

		uint16_t id;
		__asm__ volatile("movw %%fs, %0" : "=a" (id));

		return id;
	}
	size_t CPU::GetCPUCount()
	{
		return _cpuCount;
	}

	// -----
	// CPUID
	// -----

	CPUID::CPUID(uint32_t id)
	{
		int code  = static_cast<int>(id);
		__asm__ volatile("cpuid" : "=a" (_eax), "=b" (_ebx), "=c" (_ecx), "=d" (_edx) : "a" (code), "c" (0));
	}

	// -----
	// CPUInfo
	// -----

	CPUInfo::CPUInfo()
	{
		{
			CPUID cpuid(0);

			uint32_t ebx = cpuid.GetEBX();
			uint32_t edx = cpuid.GetEDX();
			uint32_t ecx = cpuid.GetECX();

			char vendor[12];
			memcpy(vendor + 0, &ebx, 4);
			memcpy(vendor + 4, &edx, 4);
			memcpy(vendor + 8, &ecx, 4);

			if(strncmp(vendor, "GenuineIntel", 12) == 0)
			{
				_vendor = CPUVendor::Intel;
			}
			else if(strncmp(vendor, "AuthenticAMD", 12) == 0)
			{
				_vendor = CPUVendor::AMD;
			}
			else
			{
				_vendor = CPUVendor::Unknown;
			}
		}

		{
			CPUID cpuid(1);

			uint32_t eax = cpuid.GetEAX();
			uint32_t edx = cpuid.GetEDX();
			uint32_t ecx = cpuid.GetECX();

			_stepping = (eax >> 0) & 0xf;
			_model    = (eax >> 4) & 0xf;
			_family   = (eax >> 8) & 0xf;
			_type     = (eax >> 12) & 0x3;

			_extendedModel  = (eax >> 16) & 0xf;
			_extendedFamily = (eax >> 20) & 0xf;

			_features = (static_cast<uint64_t>(edx) << 32) | ecx;
		}
	}

	// -----
	// Bootstrap
	// -----

	kern_return_t CPUInit()
	{
		// We need need an APIC and MSRs to run Firedrake
		CPUInfo info;

		if(!(info.GetFeatures() & CPUInfo::Feature::APIC) || !(info.GetFeatures() & CPUInfo::Feature::MSR))
		{
			kprintf("unsupported CPU (no MSR and/or APIC present)");
			return KERN_RESOURCES_MISSING;
		}

		kern_return_t result;
		if((result = ACPI::Init()) != KERN_SUCCESS)
			return result;

		if(!_bootstrapCPU)
		{
			kprintf("no bootstrap CPU");
			return KERN_FAILURE;
		}

		kprintf("%i %s", CPU::GetCPUCount(), (CPU::GetCPUCount() == 1) ? "CPU" : "CPUs");
		return KERN_SUCCESS;
	}
}
