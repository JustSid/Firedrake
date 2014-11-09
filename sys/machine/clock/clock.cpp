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
#include <libcpp/atomic.h>
#include "clock.h"

namespace Sys
{
	namespace Clock
	{
		std::atomic<uint32_t> _pitTicks = 0;

		static Sys::APIC::TimerDivisor _timerDivisor = Sys::APIC::TimerDivisor::DivideBy16;
		static uint32_t _timerResolution = 1000;
		static uint32_t _timerCount      = 0;

		APIC::TimerDivisor GetTimerDivisor()
		{
			return _timerDivisor;
		}
		uint32_t GetTimerCount()
		{
			return _timerCount;
		}
		uint32_t GetTimerResolution()
		{
			return _timerResolution;
		}


		uint32_t PITTick(uint32_t esp, __unused CPU *cpu)
		{
			_pitTicks ++;
			return esp;
		}

		void AwaitPitTicks(uint32_t ticks)
		{
			constexpr int divisor = 11931;

			outb(0x43, 0x36);
			outb(0x40, divisor & 0xff);
			outb(0x40, divisor >> 8);

			uint32_t target = _pitTicks.load() + ticks;

			while(_pitTicks.load() < target)
				CPUHalt();
		}

		uint32_t CalculateAPICFrequency(uint32_t resolution, APIC::TimerDivisor apicDivisor)
		{
			uint32_t divisor;
			switch(apicDivisor)
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
			APIC::SetTimer(apicDivisor, Sys::APIC::TimerMode::OneShot, 0xffffffff);
			APIC::ArmTimer(0xffffffff);

			// Wait...
			AwaitPitTicks(1);

			// Unarm the APIC
			APIC::UnarmTimer();

			// Calculate frequency and counter
			uint32_t count, initial;
			APIC::GetTimer(nullptr, nullptr, &initial, &count);

			uint32_t frequency = ((initial - count) + 1) * divisor * 100;
			uint32_t counter   = frequency / resolution / divisor;

			return counter;
		}

		uint32_t CalculateAPICFrequencyAverage(uint32_t resolution, Sys::APIC::TimerDivisor apic_divisor)
		{
			uint32_t accumulator = 0;

			for(int i = 0; i < 10; i++)
				accumulator += CalculateAPICFrequency(resolution, apic_divisor);

			return accumulator / 10;
		}
	}

	kern_return_t ClockInit()
	{
		SetInterruptHandler(0x20, &Clock::PITTick);
		APIC::MaskInterrupt(0x20, false);

		Clock::_timerCount = Clock::CalculateAPICFrequencyAverage(Clock::_timerResolution, Clock::_timerDivisor);

		

		return KERN_SUCCESS;
	}
}
