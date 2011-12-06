//
//  exceptions.c
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2012 by Sidney Just
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

#include "interrupts.h"
#include "panic.h"

const char *__ir_exception_pageFaultTranslateBit(int bit, int set);

cpu_state_t *__ir_exceptionHandler(cpu_state_t *state)
{	
	if(state->interrupt == 14)
	{
		// Page Fault
		uint32_t address;
		uint32_t error = state->error;

		__asm__ volatile("mov %%cr2, %0" : "=r" (address)); // Get the virtual address of the page
		panic("Page Fault occured!\nError: %s caused by %s in %s. Virtual address: %p", __ir_exception_pageFaultTranslateBit(1, error & (1 << 0)), // Panic with the type of the error
			__ir_exception_pageFaultTranslateBit(2, error & (1 << 1)), // Why it occured
			__ir_exception_pageFaultTranslateBit(3, error & (1 << 2)), // And in which mode it occured.
			address);
	}

	panic("Unhandled exception %i occured!", state->interrupt);
	return state; // We won't reach this point...
}

const char *__ir_exception_pageFaultTranslateBit(int bit, int set)
{
	switch(bit)
	{
		case 1:
			return set ? "Protection violation" : "Non present page";
			break;

		case 2:
			return set ? "write access" : "read access";
			break;

		case 3:
			return set ? "user Mode" : "supervisor Mode";
			break;

		default:
			break;
	}

	return "";
}
