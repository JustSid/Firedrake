//
//  apic.cpp
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

#include <machine/port.h>
#include <machine/memory/memory.h>
#include <kern/kprintf.h>
#include <libc/assert.h>
#include <libc/string.h>
#include <libcpp/new.h>
#include "apic.h"

#define APIC_MAX_IOAPICS 8
#define APIC_MAX_INTERRUPT_OVERRIDES 32

#define APIC_IOAPICID  0x0
#define APIC_IOAPICVER 0x1
#define APIC_IOAPICARB 0x2
#define APIC_IOREDTBL0 0x10

#define APIC_PIC_RELCOATION_OFFSET 0x20

namespace Sys
{
	// --------------------
	// MARK: -
	// MARK: IOAPIC
	// --------------------

	static char _ioapicMemory[CONFIG_MAX_IOAPICS * sizeof(IOAPIC)];
	static char _interruptOverrideMemory[CONFIG_MAX_INTERRUPT_OVERRIDES * sizeof(IOAPIC::InterruptOverride)];

	static IOAPIC *_ioapic = reinterpret_cast<IOAPIC *>(_ioapicMemory);
	static size_t _ioapicCount = 0;

	static IOAPIC::InterruptOverride *_interruptOverride = reinterpret_cast<IOAPIC::InterruptOverride *>(_interruptOverrideMemory);
	static size_t _interruptOverrideCount = 0;

	IOAPIC::IOAPIC(uint8_t id, uint32_t address, uint32_t base) :
		_id(id),
		_maxRedirectionEntry(0),
		_address(address),
		_interruptBase(base)
	{}

	IOAPIC *IOAPIC::RegisterIOAPIC(uint8_t id, uint32_t address, uint32_t base)
	{
		// Workaround for a Qemu bug(?!) where an IOAPIC id showed up multiple times in the ACPI
		for(size_t i = 0; i < _ioapicCount; i ++)
		{
			if(_ioapic[i].GetID() == id)
				return _ioapic + i;
		}

		if(_ioapicCount >= CONFIG_MAX_IOAPICS)
		{
			kprintf("Trying to register more IOAPICs than configured (increase CONFIG_MAX_IOAPICS)");
			return nullptr;
		}

		size_t index = _ioapicCount ++;

		IOAPIC *ioapic = new(&_ioapicMemory[index * sizeof(IOAPIC)]) IOAPIC(id, address, base);
		return ioapic;
	}

	IOAPIC::InterruptOverride *IOAPIC::RegisterInterruptOverride(uint8_t source, uint16_t flags, uint32_t systemInterrupt)
	{
		if(_interruptOverrideCount >= CONFIG_MAX_INTERRUPT_OVERRIDES)
		{
			kprintf("Trying to register more interrupt overrides than configured (increase CONFIG_MAX_INTERRUPT_OVERRIDES)");
			return nullptr;
		}

		size_t index = _interruptOverrideCount ++;

		InterruptOverride *override = new(&_interruptOverrideMemory[index * sizeof(InterruptOverride)]) InterruptOverride(source, flags, systemInterrupt);
		return override;
	}

	IOAPIC *IOAPIC::GetIOAPICWithID(uint8_t id)
	{
		for(size_t i = 0; i < _ioapicCount; i ++)
		{
			IOAPIC *ioapic = _ioapic + i;
			
			if(ioapic->GetID() == id)
				return ioapic;
		}

		return nullptr;
	}
	IOAPIC *IOAPIC::GetIOAPICForInterrupt(uint8_t irq)
	{
		for(size_t i = 0; i < _ioapicCount; i ++)
		{
			IOAPIC *ioapic = _ioapic + i;
			
			if(irq >= ioapic->GetInterruptBase() && irq <= ioapic->GetMaximumRedirectionEntry())
				return ioapic;
		}

		return nullptr;
	}

	uint8_t IOAPIC::ResolveInterrupt(uint8_t irq)
	{
		for(size_t i = 0; i < _interruptOverrideCount; i ++)
		{
			InterruptOverride *override = _interruptOverride + i;

			if(override->GetSource() == irq)
				return override->GetSystemInterrupt();
		}

		return irq;
	}

	IOAPIC::InterruptOverride *IOAPIC::GetInterruptOverride(uint8_t irq)
	{
		for(size_t i = 0; i < _interruptOverrideCount; i ++)
		{
			InterruptOverride *override = _interruptOverride + i;
			if(override->GetSource() == irq)
				return override;
		}

		return nullptr;
	}


	void IOAPIC::Write(Register reg, uint32_t value)
	{
		*(volatile uint32_t *)(_address) = static_cast<uint32_t>(reg);
		*(volatile uint32_t *)(_address + 0x10) = value;
	}
	uint32_t IOAPIC::Read(Register reg)
	{
		*(volatile uint32_t *)(_address) = static_cast<uint32_t>(reg);
		return *(volatile uint32_t *)(_address + 0x10);
	}

	kern_return_t IOAPIC::MapIOAPICs()
	{
		VM::Directory *directory = VM::Directory::GetKernelDirectory();
		for(size_t i = 0; i < _ioapicCount; i ++)
		{
			IOAPIC *ioapic = _ioapic + i;

			vm_address_t address;
			kern_return_t result = directory->AllocLimit(address, ioapic->_address, VM::kKernelLimit, VM::kUpperLimit, 1, kVMFlagsKernelNoCache);

			if(result != KERN_SUCCESS)
				return result;


			ioapic->_address = address;

			uint32_t version = ioapic->Read(Register::VER);
			ioapic->_maxRedirectionEntry = (version >> 16) & 0xff;
		}

		return KERN_SUCCESS;
	}

	// --------------------
	// MARK: -
	// MARK: APIC
	// --------------------

	namespace APIC
	{
		vm_address_t _apicBase = 0;
		uintptr_t _apicPhysicalBase = 0;

		void Write(Register reg, uint32_t value)
		{
			volatile uint32_t *base = reinterpret_cast<volatile uint32_t *>(_apicBase + static_cast<uint32_t>(reg));
			*base = value;
		}

		uint32_t Read(Register reg)
		{
			volatile uint32_t *base = reinterpret_cast<volatile uint32_t *>(_apicBase + static_cast<uint32_t>(reg));
			return *base;
		}


		bool IsInterruptPending()
		{
			for(int i = 0; i < 8; i ++)
			{
				if(Read(APICRegisterWithOffset(Register::IRR_BASE, i)) || Read(APICRegisterWithOffset(Register::ISR_BASE, i)))
					return true;
			}

			return false;
		}
		bool IsInterrupting(uint8_t vector)
		{
			int i = vector / 32;
			int bit = 1 << (vector % 32);

			uint32_t irr = Read(APICRegisterWithOffset(Register::IRR_BASE, i));
			uint32_t isr = Read(APICRegisterWithOffset(Register::ISR_BASE, i));

			return ((irr | isr) & bit);
		}

		// IPI
		static inline void FinishPendingIPI()
		{
			while(Read(Register::ICRL) & APIC_ICR_DS_PENDING)
				Sys::CPUPause();
		}

		void SendIPI(uint8_t vector, CPU *cpu)
		{
			SendIPI(vector, cpu, APIC_ICR_DM_FIXED);
		}

		void SendIPI(uint8_t vector, CPU *cpu, uint32_t flags)
		{
			FinishPendingIPI();
			
			Write(Register::ICRH, cpu->GetApicID() << 24);
			Write(Register::ICRL, vector | flags);
		}

		void BroadcastIPI(uint8_t vector, bool self)
		{
			BroadcastIPI(vector, self, APIC_ICR_LV_ASSERT);
		}
		void BroadcastIPI(uint8_t vector, bool self, uint32_t flags)
		{
			uint32_t deliveryMode = self ? APIC_ICR_DSS_ALL : APIC_ICR_DSS_OTHERS;

			FinishPendingIPI();

			Write(Register::ICRH, 0);
			Write(Register::ICRL, vector | flags | deliveryMode);
		}

		// Timer

		void ArmTimer(uint32_t count)
		{
			uint32_t vector = Read(Register::LVT_TIMER);
			vector &= ~APIC_LVT_MASKED;

			Write(Register::LVT_TIMER, vector);
			Write(Register::TMR_INITIAL_COUNT, count);
		}

		void UnarmTimer()
		{
			uint32_t vector = Read(APIC::Register::LVT_TIMER);
			vector |= APIC_LVT_MASKED;

			Write(Register::LVT_TIMER, vector);
		}

		void SetTimer(TimerDivisor divisor, TimerMode mode, uint32_t initial)
		{
			uint32_t vector = Read(Register::LVT_TIMER);

			vector &= ~APIC_LVT_PERIODIC;
			vector |= ((mode == TimerMode::Periodic) ? APIC_LVT_PERIODIC : 0) | APIC_LVT_MASKED;

			Write(Register::LVT_TIMER, vector);
			Write(Register::TMR_DIVIDE_CONFIG, static_cast<uint32_t>(divisor));
			Write(Register::TMR_INITIAL_COUNT, initial);
		}

		void GetTimer(TimerDivisor *divisor, TimerMode *mode, uint32_t *initial, uint32_t *current)
		{
			uint32_t timeVector = Read(Register::LVT_TIMER);

			if(mode)
				*mode = (timeVector & APIC_LVT_PERIODIC) ? TimerMode::Periodic : TimerMode::OneShot;

			if(divisor)
				*divisor = static_cast<TimerDivisor>(Read(Register::TMR_DIVIDE_CONFIG));

			if(initial)
				*initial = Read(Register::TMR_INITIAL_COUNT);

			if(current)
				*current = Read(Register::TMR_CURRENT_COUNT);
		}

		// Misc

		uintptr_t GetBase()
		{
			uint64_t value = Sys::CPUReadMSR(0x1b);
			uint32_t base  = static_cast<uint32_t>(value & ~0xfff);

			return base;
		}

		kern_return_t MapBase()
		{
			kern_return_t result;
			vm_address_t vaddress;

			uintptr_t paddress = GetBase();

			result = VM::Directory::GetKernelDirectory()->AllocLimit(vaddress, paddress, VM::kKernelLimit, VM::kUpperLimit, 1, kVMFlagsKernelNoCache);
			if(result != KERN_SUCCESS)
			{
				kprintf("couldn't allocate APIC memory");
				return result;
			}

			_apicPhysicalBase = paddress;
			_apicBase         = vaddress;

			return KERN_SUCCESS;
		}

		kern_return_t MapBaseAP()
		{
			uintptr_t paddress = GetBase();
			if(paddress != _apicPhysicalBase) // Just in case an AP has a different APIC base
			{
				uint64_t value = Sys::CPUReadMSR(0x1b) & 0xfff;
				value = _apicPhysicalBase | value;

				Sys::CPUWriteMSR(0x1b, value);
			}

			return KERN_SUCCESS;
		}

		uint32_t Vector(uint8_t interrupt, bool masked)
		{
			uint32_t vector = interrupt;
			if(masked)
				vector |= APIC_LVT_MASKED;

			return vector;
		}

		void MaskInterrupt(uint8_t vector, bool masked)
		{
			assert(vector >= 0x20 && vector <= 0x2f);

			uint8_t irq   = vector - APIC_PIC_RELCOATION_OFFSET;
			uint32_t data = masked ? (vector | (1 << 16)) : vector;

			IOAPIC::InterruptOverride *override = IOAPIC::GetInterruptOverride(irq);
			if(override)
			{
				irq = override->GetSystemInterrupt();

				data |= override->IsLevelTriggered() ? (1 << 15) : 0;
				data |= override->IsActiveHigh() ? 0 : (1 << 13);
			}

			uint32_t offset = static_cast<uint32_t>(IOAPIC::Register::REDTBL0) + (irq * 2);
			IOAPIC *ioapic  = IOAPIC::GetIOAPICForInterrupt(irq);

			assert(ioapic);

			ioapic->Write(static_cast<IOAPIC::Register>(offset + 0), data);
			ioapic->Write(static_cast<IOAPIC::Register>(offset + 1), 0);
		}
	}

	void MaskPIC()
	{
		// The PIC is completely masked, so there shouldn't be any interrupts
		// Well, BOCHS thinks differently, so we remap the interrupts to avoid the PIT showing up
		// as double fault...
		outb(0x20, 0x11);
		outb(0x21, 0x20);
		outb(0x21, 0x04);
		outb(0x21, 0x01);

		outb(0xa0, 0x11);
		outb(0xa1, 0x28);
		outb(0xa1, 0x02);
		outb(0xa1, 0x01);

		outb(0x21, 0xff);
		outb(0xa1, 0xff);
	}

	kern_return_t APICInit()
	{
		kern_return_t result;
		MaskPIC();

		if((result = APIC::MapBase()) != KERN_SUCCESS)
		{
			kprintf("failed to map APIC");
			return result;
		}

		if((result = IOAPIC::MapIOAPICs()) != KERN_SUCCESS)
		{
			kprintf("failed to map I/O APIC(s)");
			return result;
		}
		
		return APICInitCPU();
	}

	kern_return_t APICInitCPU()
	{
		kern_return_t result;

		if((result = APIC::MapBaseAP()) != KERN_SUCCESS)
		{
			kprintf("failed to map APIC");
			return result;
		}

		// Initialize the APIC
		APIC::Write(APIC::Register::TPR, 0);
		APIC::Write(APIC::Register::SVR, (1 << 8) | 0x32);

		APIC::Write(APIC::Register::DFR, APIC_DFR_FLAT);
		APIC::Write(APIC::Register::LDR, Sys::CPU::GetCPUID() << 24);

		APIC::Write(APIC::Register::LVT_PMC,     APIC::Vector(0x30, false));
		//apic_write(APIC::Register::LVT_CMCI,    APIC::Vector(0x31, false));
		APIC::Write(APIC::Register::LVT_ERROR,   APIC::Vector(0x33, false));
		APIC::Write(APIC::Register::LVT_THERMAL, APIC::Vector(0x34, false));
		APIC::Write(APIC::Register::LVT_TIMER,   APIC::Vector(0x35, false));

		APIC::Write(APIC::Register::LVT_LINT0,   APIC::Vector(0x37, false));
		APIC::Write(APIC::Register::LVT_LINT1,   APIC::Vector(0x38, false));

		return KERN_SUCCESS;
	}
}
