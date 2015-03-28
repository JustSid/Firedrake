//
//  debug.cpp
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

#include <machine/interrupts/interrupts.h>
#include <libc/backtrace.h>
#include <kern/kprintf.h>
#include "debug.h"

namespace Sys
{
	char TranslateCondition(WatchpointCondition condition)
	{
		switch(condition)
		{
			case WatchpointCondition::OnExecution:
				return 0;
			case WatchpointCondition::OnWrite:
				return 1;
			case WatchpointCondition::OnReadWrite:
				return 3;
			default:
				kprintf("Unsupported condition %i, defaulting to on execution!\n", condition);
				return 0;
		}
	}
	char TranslateBytes(int bytes)
	{
		switch(bytes)
		{
			case 1:
				return 0;
			case 2:
				return 1;
			case 4:
				return 3;
			case 8:
				return 2;
			default:
				kprintf("Unsupported byte range %i, defaulting to 8!\n", bytes);
				return 2;
		}
	}


	void SetWatchpoint(uint8_t reg, bool global, uintptr_t address, WatchpointCondition tcondition, int tbytes)
	{
		uint32_t dr7;
		__asm__ volatile("mov %%dr7, %0" : "=r" (dr7));

		char condition = TranslateCondition(tcondition);
		char bytes = TranslateBytes(tbytes);

		dr7 &= ~(3 << reg);
		dr7 &= ~(15 << (16 + reg * 4));

		dr7 |= (1 << (reg + (global ? 1 : 0)));
		dr7 |= (condition << (16 + reg * 4));
		dr7 |= (bytes << (18 + reg * 4));


		switch(reg)
		{
			case 0:
				__asm__ volatile("mov %0, %%dr0" : : "r" (address));
				break;
			case 1:
				__asm__ volatile("mov %0, %%dr1" : : "r" (address));
				break;
			case 2:
				__asm__ volatile("mov %0, %%dr2" : : "r" (address));
				break;
			case 3:
				__asm__ volatile("mov %0, %%dr3" : : "r" (address));
				break;

			default:
				break;
		}

		__asm__ volatile("mov %0, %%dr7" : : "r" (dr7));
	}

	void DisableWatchpoint(uint8_t reg)
	{
		uint32_t dr7;
		__asm__ volatile("mov %%dr7, %0" : "=r" (dr7));

		dr7 &= ~(3 << reg);
		dr7 &= ~(15 << (16 + reg * 4));

		__asm__ volatile("mov %0, %%dr7" : : "r" (dr7));
	}



	uint32_t HandleWatchpoint(uint32_t esp, Sys::CPU *cpu)
	{
		Sys::CPUState *state = cpu->GetLastState();

		uint32_t dr6;
		__asm__ volatile("mov %%dr6, %0" : "=r" (dr6));

		int watchpoint;
		uintptr_t address;

#define CheckWatchpoint(index) \
		do { \ 
			if(dr6 & (1 << index)) \
			{ \
				__asm__ volatile("mov %%dr" #index ", %0" : "=r" (address)); \
				watchpoint = index; \
			} } while(0)

		CheckWatchpoint(0);
		CheckWatchpoint(1);
		CheckWatchpoint(2);
		CheckWatchpoint(3);


		kprintf("Watchpoint #%i triggered. Address: %p, backtrace:\n", watchpoint, address);

		void *buffer[10];
		int count = backtrace_np(reinterpret_cast<void *>(state->ebp), buffer, 10);

		for(int i = 0; i < count; i ++)
			kprintf("(%2i) %08x\n", (count - 1) - i, reinterpret_cast<uint32_t>(buffer[i]));

		__asm__ volatile("mov %0, %%dr6" : : "r" (0));
		return esp;
	}
}
