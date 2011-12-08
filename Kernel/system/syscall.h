//
//  syscall.h
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

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <types.h>
#include "cpu.h"

typedef enum
{
	syscall_print,
	syscall_printColor,

	__syscall_max // NOT an actuall syscall but just used as boundary information!
} syscall_t;


typedef uint32_t (*syscall_callback_t)(cpu_state_t *state, cpu_state_t **returnState);

uint32_t syscall0(syscall_t type);
uint32_t syscall1(syscall_t type, uint32_t arg1);
uint32_t syscall2(syscall_t type, uint32_t arg1, uint32_t arg2);
uint32_t syscall3(syscall_t type, uint32_t arg1, uint32_t arg2, uint32_t arg3);

void sc_mapSyscall(syscall_t syscall, syscall_callback_t callback);
bool sc_init(void *ingored);

#endif
