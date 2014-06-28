//
//  clock.cpp
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
#include <kern/kprintf.h>
#include "clock.h"

namespace clock
{
	volatile uint32_t _clock_pit_ticks = 0;

	Sys::APIC::TimerDivisor default_time_divisor = Sys::APIC::TimerDivisor::DivideBy16;
	uint32_t default_time_resolution = 1000;
	uint32_t default_time_counter    = 0;

	// PIT

	uint32_t pit_tick(uint32_t esp, __unused Sys::CPU *cpu)
	{
		_clock_pit_ticks ++;
		return esp;
	}

	void await_pit_ticks(uint32_t ticks)
	{
		constexpr int pit_divisor = 11931;

		outb(0x43, 0x36);
		outb(0x40, pit_divisor & 0xff);
		outb(0x40, pit_divisor >> 8);

		uint32_t target = _clock_pit_ticks + ticks;

		while(_clock_pit_ticks < target)
			Sys::CPUHalt();
	}

	void init_pit()
	{
		Sys::SetInterruptHandler(0x20, &pit_tick);
		Sys::APIC::MaskInterrupt(0x20, false);
	}	

	// APIC

	uint32_t calculate_apic_frequency(uint32_t resolution, Sys::APIC::TimerDivisor apic_divisor)
	{
		uint32_t divisor;
		switch(apic_divisor)
		{
			case Sys::APIC::TimerDivisor::DivideBy1:
				divisor = 1;
				break;
			case Sys::APIC::TimerDivisor::DivideBy2:
				divisor = 2;
				break;
			case Sys::APIC::TimerDivisor::DivideBy4:
				divisor = 4;
				break;
			case Sys::APIC::TimerDivisor::DivideBy8:
				divisor = 8;
				break;
			case Sys::APIC::TimerDivisor::DivideBy16:
				divisor = 16;
				break;
			case Sys::APIC::TimerDivisor::DivideBy32:
				divisor = 32;
				break;
			case Sys::APIC::TimerDivisor::DivideBy64:
				divisor = 64;
				break;
			case Sys::APIC::TimerDivisor::DivideBy128:
				divisor = 128;
				break; 
		}

		// Prepare the APIC timer
		Sys::APIC::SetTimer(apic_divisor, Sys::APIC::TimerMode::OneShot, 0xffffffff);
		Sys::APIC::ArmTimer(0xffffffff);

		// Wait...
		await_pit_ticks(1);

		// Unarm the APIC
		Sys::APIC::UnarmTimer();

		// Calculate frequency and counter
		uint32_t count, initial;
		Sys::APIC::GetTimer(nullptr, nullptr, &initial, &count);

		uint32_t frequency = ((initial - count) + 1) * divisor * 100;
		uint32_t counter   = frequency / resolution / divisor;

		return counter;
	}

	uint32_t calculate_apic_frequency_average(uint32_t resolution, Sys::APIC::TimerDivisor apic_divisor)
	{
		uint32_t accumulator = 0;

		for(int i = 0; i < 10; i++)
			accumulator += calculate_apic_frequency(resolution, apic_divisor);

		return accumulator / 10;
	}

	kern_return_t init()
	{
		init_pit();

		default_time_counter = calculate_apic_frequency_average(default_time_resolution, default_time_divisor);

		return KERN_SUCCESS;
	}
}
