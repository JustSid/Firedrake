//
//  apic.cpp
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

#include <machine/port.h>
#include <machine/memory/memory.h>
#include <kern/kprintf.h>
#include <libc/string.h>
#include "apic.h"

#define APIC_MAX_IOAPICS 8
#define APIC_MAX_INTERRUPT_OVERRIDES 32

#define APIC_IOAPICID  0x0
#define APIC_IOAPICVER 0x1
#define APIC_IOAPICARB 0x2
#define APIC_IOREDTBL0 0x10

namespace ir
{
	// --------------------
	// MARK: -
	// MARK: ioapic_interrupt_override_t
	// --------------------

	ioapic_interrupt_override_t _apic_interrupt_override[APIC_MAX_INTERRUPT_OVERRIDES];
	size_t _apic_interrupt_override_count = 0;

	bool ioapic_interrupt_override_t::is_active_high() const
	{
		uint16_t polarity = (flags & 0x3);
		return (polarity == 1); 
	}

	bool ioapic_interrupt_override_t::is_active_low() const
	{
		return (!is_active_high());
	}

	bool ioapic_interrupt_override_t::is_level_triggered() const
	{
		uint16_t trigger = ((flags >> 2) & 0x3);
		return (trigger == 3);
	}

	bool ioapic_interrupt_override_t::is_edge_triggered() const
	{
		return (!is_level_triggered());
	}

	ioapic_interrupt_override_t *apic_ioapic_get_interrupt_override(uint8_t irq)
	{
		for(size_t i = 0; i < _apic_interrupt_override_count; i ++)
		{
			if(_apic_interrupt_override[i].source == irq)
				return &_apic_interrupt_override[i];
		}

		return nullptr;
	}

	uint8_t apic_iopaic_resolve_interrupt(uint8_t irq)
	{
		ioapic_interrupt_override_t *override = apic_ioapic_get_interrupt_override(irq);
		if(override)
			return static_cast<uint8_t>(override->system_interrupt);

		return irq;
	}

	kern_return_t apic_add_interrupt_override(ioapic_interrupt_override_t *override)
	{
		if(_apic_interrupt_override_count >= APIC_MAX_INTERRUPT_OVERRIDES)
			return KERN_NO_MEMORY;

		memcpy(&_apic_interrupt_override[_apic_interrupt_override_count], override, sizeof(ioapic_interrupt_override_t));
		_apic_interrupt_override_count ++;

		return KERN_SUCCESS;
	}

	// --------------------
	// MARK: -
	// MARK: I/O APIC
	// --------------------

	ioapic_t _apic_ioapic[APIC_MAX_IOAPICS];
	size_t _apic_ioapic_count = 0;

	kern_return_t apic_add_ioapic(ioapic_t *ioapic)
	{
		if(_apic_ioapic_count >= APIC_MAX_IOAPICS)
			return KERN_NO_MEMORY;

		memcpy(&_apic_ioapic[_apic_ioapic_count], ioapic, sizeof(ioapic_t));
		_apic_ioapic_count ++;

		return KERN_SUCCESS;
	}

	void apic_write_ioapic(ioapic_t *ioapic, uint8_t offset, uint32_t value)
	{
		*(uint32_t *)(ioapic->address) = offset;
		*(uint32_t *)(ioapic->address + 0x10) = value;
	}

	uint32_t apic_read_ioapic(ioapic_t *ioapic, uint8_t offset)
	{
		*(uint32_t *)(ioapic->address) = offset;
		return *(uint32_t *)(ioapic->address + 0x10);
	}

	kern_return_t apic_map_ioapics()
	{
		vm::directory *directory = vm::get_kernel_directory();
		for(size_t i = 0; i < _apic_ioapic_count; i ++)
		{
			vm_address_t address;
			kern_return_t result = directory->alloc_limit(address, _apic_ioapic[i].address, VM_KERNEL_LIMIT, VM_UPPER_LIMIT, 1, VM_FLAGS_KERNEL_NOCACHE);

			if(result != KERN_SUCCESS)
				return result;

			_apic_ioapic[i].address = address;

			uint32_t version = apic_read_ioapic(&_apic_ioapic[i], APIC_IOAPICVER);
			_apic_ioapic[i].maxiumum_redirection_entry = (version >> 16) & 0xff;
		}

		return KERN_SUCCESS;
	}

	ioapic_t *apic_ioapic_servicing_interrupt(uint8_t irq)
	{
		for(size_t i = 0; i < _apic_ioapic_count; i ++)
		{
			ioapic_t *ioapic = &_apic_ioapic[i];

			if(ioapic->interrupt_base >= irq && ioapic->interrupt_base + ioapic->maxiumum_redirection_entry < irq)
				return ioapic;
		}

		return nullptr;
	}

	void apic_ioapic_set_interrupt(uint8_t irq, uint8_t vector, bool masked)
	{
		uint32_t data = vector;
		data |= masked ? (1 << 16) : 0;

		ioapic_interrupt_override_t *override = apic_ioapic_get_interrupt_override(irq);
		if(override)
		{
			irq = override->system_interrupt;

			data |= override->is_level_triggered() ? (1 << 15) : 0;
			data |= override->is_active_low() ? (1 << 13) : 0;
		}

		uint8_t offset = APIC_IOREDTBL0 + (irq * 2);
		ioapic_t *ioapic = &_apic_ioapic[0];

		apic_write_ioapic(ioapic, offset + 0, data);
		apic_write_ioapic(ioapic, offset + 1, 0);
	}

	// --------------------
	// MARK: -
	// MARK: APIC
	// --------------------

	vm_address_t _apic_base = 0;
	uintptr_t _apic_physical_base = 0;

	void apic_write(uint32_t reg, uint32_t value)
	{
		uint32_t *base = reinterpret_cast<uint32_t *>(_apic_base + reg);
		*base = value;
	}

	uint32_t apic_read(uint32_t reg)
	{
		uint32_t *base = reinterpret_cast<uint32_t *>(_apic_base + reg);
		return *base;
	}

	bool apic_is_interrupt_pending()
	{
		for(int i = 0; i < 8; i++)
		{
			if(apic_read(APIC_REGISTER_IRR_BASE + i) || apic_read(APIC_REGISTER_ISR_BASE + i))
				return true;
		}

		return true;
	}

	bool apic_is_interrupting(uint8_t vector)
	{
		int i = vector / 32;
		int bit = 1 << (vector % 32);

		uint32_t irr = apic_read(APIC_REGISTER_IRR_BASE + i);
		uint32_t isr = apic_read(APIC_REGISTER_ISR_BASE + i);

		return ((irr | isr) & bit);
	}


	uintptr_t apic_get_base()
	{
		uint64_t value = cpu_read_msr(0x1b);
		uint32_t base  = static_cast<uint32_t>(value & ~0xfff);

		return base;
	}

	kern_return_t apic_map_base()
	{
		kern_return_t result;
		vm_address_t vaddress;

		uintptr_t paddress = apic_get_base();

		result = vm::get_kernel_directory()->alloc_limit(vaddress, paddress, VM_KERNEL_LIMIT, VM_UPPER_LIMIT, 1, VM_FLAGS_KERNEL_NOCACHE);
		if(result != KERN_SUCCESS)
		{
			kprintf("couldn't allocate apic memory");
			return result;
		}

		_apic_physical_base = paddress;
		_apic_base = vaddress;

		return KERN_SUCCESS;
	}

	uint32_t apic_vector(uint8_t interrupt, bool masked)
	{
		uint32_t vector = interrupt;
		if(masked)
			vector |= (1 << 16);

		return vector;
	}

	// --------------------
	// MARK: -
	// MARK: Timer
	// --------------------

	void apic_set_timer(bool masked, apic_timer_divisor_t divisor, apic_timer_mode_t mode, uint32_t initial_count)
	{
		uint32_t timeVector = apic_read(APIC_REGISTER_LVT_TIMER);

		timeVector &= ~(APIC_LVT_MASKED | APIC_LVT_PERIODIC);
		timeVector |= masked ? APIC_LVT_MASKED : 0;
		timeVector |= (mode == apic_timer_mode_t::periodic) ? APIC_LVT_PERIODIC : 0;

		apic_write(APIC_REGISTER_LVT_TIMER, timeVector);
		apic_write(APIC_REGISTER_TIMER_DIVIDE_CONFIG, static_cast<uint32_t>(divisor));
		apic_write(APIC_REGISTER_TIMER_INITAL_COUNT, initial_count);
	}

	void apic_get_timer(apic_timer_divisor_t *divisor, apic_timer_mode_t *mode, uint32_t *initial, uint32_t *current)
	{
		uint32_t timeVector = apic_read(APIC_REGISTER_LVT_TIMER);

		if(mode)
			*mode = (timeVector & APIC_LVT_PERIODIC) ? apic_timer_mode_t::periodic : apic_timer_mode_t::one_shot;

		if(divisor)
			*divisor = static_cast<apic_timer_divisor_t>(apic_read(APIC_REGISTER_TIMER_DIVIDE_CONFIG));

		if(initial)
			*initial = apic_read(APIC_REGISTER_TIMER_INITAL_COUNT);

		if(current)
			*current = apic_read(APIC_REGISTER_TIMER_CURRENT_COUNT);
	}

	// --------------------
	// MARK: -
	// MARK: Initialization
	// --------------------

	void apic_mask_pic()
	{
		outb(0x21, 0xff);
		outb(0xa1, 0xff);
	}

	kern_return_t apic_init()
	{
		kern_return_t result;
		apic_mask_pic();

		if((result = apic_map_base()) != KERN_SUCCESS)
		{
			kprintf("failed to map APIC");
			return result;
		}

		if((result = apic_map_ioapics()) != KERN_SUCCESS)
		{
			kprintf("failed to map I/O APIC(s)");
			return result;
		}

		// Initialize the APIC
		apic_write(APIC_REGISTER_TPR, 0);
		apic_write(APIC_REGISTER_SVR, (1 << 8) | 0x32);

		apic_write(APIC_REGISTER_LVT_PMC,     apic_vector(0x30, true));
		apic_write(APIC_REGISTER_LVT_CMCI,    apic_vector(0x31, true));
		apic_write(APIC_REGISTER_LVT_ERROR,   apic_vector(0x33, true));
		apic_write(APIC_REGISTER_LVT_THERMAL, apic_vector(0x34, true));
		apic_write(APIC_REGISTER_LVT_TIMER,   apic_vector(0x35, true));

		apic_write(APIC_REGISTER_LVT_LINT0,   apic_vector(0x37, false));
		apic_write(APIC_REGISTER_LVT_LINT1,   apic_vector(0x38, false));

		return KERN_SUCCESS;
	}
}
