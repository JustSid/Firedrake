//
//  clock.cpp
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
#include <kern/kprintf.h>
#include "clock.h"

namespace clock
{
	volatile uint32_t _clock_pit_ticks = 0;

	ir::apic_timer_divisor_t default_time_divisor = ir::apic_timer_divisor_t::divide_by_16;
	uint32_t default_time_resolution = 1000;
	uint32_t default_time_counter = 0;


	uint32_t clock_pit_tick(uint32_t esp, __unused cpu_t *cpu)
	{
		_clock_pit_ticks ++;
		return esp;
	}

	uint32_t calculate_apic_frequency(uint32_t resolution, ir::apic_timer_divisor_t apic_divisor)
	{
		uint32_t divisor;
		switch(apic_divisor)
		{
			case ir::apic_timer_divisor_t::divide_by_1:
				divisor = 1;
				break;
			case ir::apic_timer_divisor_t::divide_by_2:
				divisor = 2;
				break;
			case ir::apic_timer_divisor_t::divide_by_4:
				divisor = 4;
				break;
			case ir::apic_timer_divisor_t::divide_by_8:
				divisor = 8;
				break;
			case ir::apic_timer_divisor_t::divide_by_16:
				divisor = 16;
				break;
			case ir::apic_timer_divisor_t::divide_by_32:
				divisor = 32;
				break;
			case ir::apic_timer_divisor_t::divide_by_64:
				divisor = 64;
				break;
			case ir::apic_timer_divisor_t::divide_by_128:
				divisor = 128;
				break; 
		}

		// Prepare the APIC timer and interrupts
		ir::apic_set_timer(apic_divisor, ir::apic_timer_mode_t::one_shot, 0xffffffff);
		ir::apic_ioapic_mask_interrupt(0x20, false);
		ir::set_interrupt_handler(0x20, &clock_pit_tick);

		// Arm the PIT at 100hz
		int pit_divisor = 11931;
		outb(0x43, 0x36);
		outb(0x40, pit_divisor & 0xff);
		outb(0x40, pit_divisor >> 8);

		// Arm the APIC timer
		ir::apic_arm_timer(0xffffffff);

		// Wait...
		uint32_t target = _clock_pit_ticks + 1;
		while(_clock_pit_ticks < target)
		{}

		// Unarm the APIC timer and disable interrupts
		ir::apic_unarm_timer();
		ir::set_interrupt_handler(0x20, nullptr);
		ir::apic_ioapic_mask_interrupt(0x20, true);

		// Calculate frequency and counter
		uint32_t count;
		uint32_t initial;
		ir::apic_get_timer(nullptr, nullptr, &initial, &count);

		uint32_t frequency = ((initial - count) + 1) * divisor * 100;
		uint32_t counter = frequency / resolution / divisor;

		return counter;
	}

	uint32_t calculate_apic_frequency_average(uint32_t resolution, ir::apic_timer_divisor_t apic_divisor)
	{
		uint32_t accumulator = 0;

		for(int i = 0; i < 10; i++)
			accumulator += calculate_apic_frequency(resolution, apic_divisor);
		
		return accumulator / 10;
	}


	kern_return_t init()
	{
		default_time_counter = calculate_apic_frequency_average(default_time_resolution, default_time_divisor);
		return KERN_SUCCESS;
	}
}
