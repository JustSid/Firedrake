//
//  smp.cpp
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

#include <kern/kprintf.h>
#include <libc/sys/spinlock.h>
#include <libc/string.h>
#include <libc/assert.h>
#include <machine/port.h>
#include <machine/cme.h>
#include <machine/gdt.h>
#include <machine/clock/clock.h>
#include "smp.h"

extern "C" void smp_rendezvous_point();
extern "C" uintptr_t smp_bootstrap_begin;
extern "C" uintptr_t smp_bootstrap_end;
extern "C" uint32_t *_kernelPageDirectory;

#define SMP_PHYSICAL_CODE 0x7000
#define SMP_PHYSICAL_GDT  0x8000

namespace Sys
{
	void GDTSetEntry(uint64_t *gdt, int16_t index, uint32_t base, uint32_t limit, int32_t flags);

	namespace SMP
	{
		/*
			entryBitmap uses the following code (generated by nasm)
			offsets and targets are patched up later

			00000000  FA                  cli
			00000001  0F 01 16 00 80      lgdt [0x8000]
			00000006  0F 20 C0            mov eax,cr0
			00000009  66 83 C8 01         or eax,byte +0x1
			0000000D  0F 22 C0            mov cr0,eax
			00000010  EA 00 00 08 00      jmp word 0x8:0x0000
		*/

		static uint8_t entryBitmap[] = {
			0xfa,
			0x0f, 0x01, 0x16, 0x00, 0x80,
			0x0f, 0x20, 0xc0,
			0x66, 0x83, 0xc8, 0x01,
			0x0f, 0x22, 0xc0,
			0xea, 0x00, 0x00, 0x08, 0x00
		};
		static spinlock_t _rendezvousLock = SPINLOCK_INIT;

		void RendezvousFail()
		{
			// Just an idle point for the CPU. We missed the sync point, but it's already running
			// so it's just going to idle from now on

			__asm__ volatile("sti");
			spinlock_unlock(&_rendezvousLock);

			while(1)
				CPUHalt();
		}

		void RendezvousPoint()
		{
			// Activate the kernel directory and virtual memory
			uint32_t cr0;
			__asm__ volatile("mov %0, %%cr3" : : "r" (reinterpret_cast<uint32_t>(_kernelPageDirectory)));
			__asm__ volatile("mov %%cr0, %0" : "=r" (cr0));
			__asm__ volatile("mov %0, %%cr0" : : "r" (cr0 | (1 << 31)));

			spinlock_lock(&_rendezvousLock);

			// Bootstrap Interrupts
			if(!InterruptsInitAP().IsValid())
				RendezvousFail();
			
			CPU *cpu = CPU::GetCurrentCPU();

			if(cpu->GetFlags() & CPU::Flags::Timedout)
			{
				// Well, too late...
				RendezvousFail();
			}

			cpu->Bootstrap();
			spinlock_unlock(&_rendezvousLock);

			// Wait for work. The CPU will receive an IPI once the kernel is ready to start scheduling
			// this will automatically switch stacks and stop this thread of execution
			while(1)
				CPUHalt();
		}

		void KickoffCPU(CPU *cpu)
		{
			uint32_t vector = APIC_ICR_DM_INIT | APIC_ICR_LV_ASSERT;
			APIC::SendIPI(0x0f, cpu, vector);

			// Wait 10ms for the CPU to initialize
			Clock::AwaitPITTicks(1);

			vector = ((SMP_PHYSICAL_CODE / 0x1000) & 0xff) | APIC_ICR_DSS_OTHERS | APIC_ICR_DM_STARTUP | APIC_ICR_LV_ASSERT;
			APIC::SendIPI(0x0, cpu, vector);

			// Wait up to 100ms for the rendezvous to happen
			for(int i = 0; i < 10; i ++)
			{
				Clock::AwaitPITTicks(1);
				CPUHalt();

				if(spinlock_try_lock(&_rendezvousLock))
				{
					if(cpu->GetFlags() & CPU::Flags::Running)
					{
						spinlock_unlock(&_rendezvousLock);
						break;
					}

					spinlock_unlock(&_rendezvousLock);
				}
			}

			// Synchronize with the CPU to avoid race conditions
			spinlock_lock(&_rendezvousLock);

			if(!(cpu->GetFlags() & CPU::Flags::Running))
				cpu->SetFlags(cpu->GetFlags() | CPU::Flags::Timedout);

			spinlock_unlock(&_rendezvousLock);
		}

		void Kickoff()
		{
			bool wasActive = Clock::ActivatePIT();

			// Start up all CPUs
			size_t count = CPU::GetCPUCount();
			for(size_t i = 0; i < count; i ++)
			{
				CPU *cpu = CPU::GetCPUWithID(i);

				if(cpu->GetFlags() & CPU::Flags::Running)
					continue;

				KickoffCPU(cpu);
			}

			if(!wasActive)
				Clock::DeactivatePIT();
		}

		void ForgeGDT(uint8_t *buffer)
		{
			struct 
			{
				uint16_t limit;
				void *pointer;
			} __attribute__((packed)) gdtp;

			gdtp.limit   = 0x18;
			gdtp.pointer = reinterpret_cast<void *>(SMP_PHYSICAL_GDT + sizeof(gdtp));

			memcpy(buffer, &gdtp, sizeof(gdtp));

			uint64_t *gdt = reinterpret_cast<uint64_t *>(buffer + sizeof(gdtp));

			GDTSetEntry(gdt, 0, 0x0, 0x0, 0);
			GDTSetEntry(gdt, 1, 0x0, 0xffffffff, GDT_FLAG_SEGMENT | GDT_FLAG_32_BIT | GDT_FLAG_CODESEG | GDT_FLAG_4K | GDT_FLAG_PRESENT);
			GDTSetEntry(gdt, 2, 0x0, 0xffffffff, GDT_FLAG_SEGMENT | GDT_FLAG_32_BIT | GDT_FLAG_DATASEG | GDT_FLAG_4K | GDT_FLAG_PRESENT);
		}
	}

	KernReturn<void> SMPInit()
	{
		// Get space to set up the protected mode GDT and the bootstrapping code
		uint8_t *bootstrapBegin = reinterpret_cast<uint8_t *>(&smp_bootstrap_begin);
		uint8_t *bootstrapEnd   = reinterpret_cast<uint8_t *>(&smp_bootstrap_end);

		size_t size  = (bootstrapEnd - bootstrapBegin) + sizeof(SMP::entryBitmap);
		size_t pages = VM_PAGE_COUNT(size) + 1;

		KernReturn<vm_address_t> vaddress;

		if((vaddress = VM::Directory::GetKernelDirectory()->Alloc(SMP_PHYSICAL_CODE, pages, kVMFlagsKernel)).IsValid() == false)
		{
			kprintf("failed to allocate virtual memory");
			return vaddress.GetError();
		}

		// Copy the bootstrapping code and set up the GDT
		uint8_t *buffer = reinterpret_cast<uint8_t *>(vaddress.Get());	
		SMP::ForgeGDT(buffer + VM_PAGE_SIZE);

		memcpy(buffer, SMP::entryBitmap, sizeof(SMP::entryBitmap));
		memcpy(buffer + sizeof(SMP::entryBitmap), bootstrapBegin, size);

		// Fix up offsets
		cme_fix_ljump16(buffer, size, reinterpret_cast<void *>(SMP_PHYSICAL_CODE + sizeof(SMP::entryBitmap)));

		// Unmap the buffer and kick off the CPUs
		VM::Directory::GetKernelDirectory()->Free(vaddress, pages);
		SMP::Kickoff();

		// Debug output
		size_t count = CPU::GetCPUCount();
		for(size_t i = 0; i < count; i ++)
		{
			CPU *cpu = CPU::GetCPUWithID(i);
			CPU::Flags flags = cpu->GetFlags();

			kprintf("%c", (flags & CPU::Flags::Running) ? (flags & CPU::Flags::Bootstrap) ? 'B' : 'R' : 'T');
		}

		return ErrorNone;
	}
}

void smp_rendezvous_point()
{
	Sys::SMP::RendezvousPoint();
}
