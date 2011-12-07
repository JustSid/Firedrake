//
//  syscall.c
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

#include "syscall.h"
#include "assert.h"
#include "interrupts.h"
#include "video.h"

syscall_callback_t _sc_syscalls[__syscall_max];

// MARK: Syscall caller
uint32_t syscall0(syscall_t type)
{
	uint32_t retVal;
	
	__asm__ volatile("mov %1, %%eax;"
					 "int $0x30;" : "=a" (retVal) : "r" (type));
	
	return retVal;
}

uint32_t syscall1(syscall_t type, uint32_t arg1)
{
	uint32_t retVal = 0;
	
	__asm__ volatile("pushl %2;"
					 "movl %1, %%eax;"
					 "int $0x30;"
					 "add $0x4, %%esp;" : "=a" (retVal) : "r" (type), "r" (arg1));
	
	return retVal;
}

uint32_t syscall2(syscall_t type, uint32_t arg1, uint32_t arg2)
{
	uint32_t retVal = 0;
	
	__asm__ volatile("pushl %3;"
					 "pushl %2;"
					 "movl %1, %%eax;"
					 "int $0x30;"
					 "add $0x8, %%esp;" : "=a" (retVal) : "r" (type), "r" (arg1), "r" (arg2));
	
	return retVal;
}

uint32_t syscall3(syscall_t type, uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	uint32_t retVal = 0;
	
	__asm__ volatile("pushl %4;"
					 "pushl %3;"
					 "pushl %2;"
					 "movl %1, %%eax;"
					 "int $0x30;"
					 "add $0xC, %%esp;" : "=a" (retVal) : "r" (type), "r" (arg1), "r" (arg2), "r" (arg3));
	
	return retVal;
}



// Mark: Implementation

uint32_t _sc_print(cpu_state_t *state, cpu_state_t **returnState)
{
	char *string = *(char **)(state->esp);
	vd_color_t color = vd_color_lightGray;

	if(state->eax == syscall_printColor)
		color = *((vd_color_t *)(state->esp + 4));

	vd_printString(string, color);
	return 0;
}


// Mark: Setup
cpu_state_t *_sc_execute(cpu_state_t *state)
{
	uint32_t call = state->eax;
	
	cpu_state_t *returnState = state;
	syscall_callback_t callback = _sc_syscalls[call];
	
	state->eax = callback(state, &returnState);
	return returnState;
}

void sc_mapSyscall(syscall_t syscall, syscall_callback_t callback)
{
	assert(syscall >= 0 && syscall < __syscall_max);
	_sc_syscalls[syscall] = callback;
}

bool sc_init(void *ingored)
{
	ir_setInterruptHandler(_sc_execute, 0x30);

	sc_mapSyscall(syscall_print, _sc_print);
	sc_mapSyscall(syscall_printColor, _sc_print);

	return true;
}
