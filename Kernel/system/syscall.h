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
	syscall_print, // syscall 1
	syscall_printColor, // syscall 2
	syscall_sleep, // Syscall 0
	syscall_processCreate, // syscall 1 (function)
	syscall_threadAttach, // syscall 2 (function, priority)
	syscall_threadJoin, // syscall 1 (thread ID)

	__syscall_max // NOT an actuall syscall but just used as boundary information!
} syscall_t;
	
#define sys_print(string) syscall1(syscall_print, (uint32_t)string)
#define sys_printColor(string, color) syscall2(syscall_printColor, (uint32_t)string, (uint32_t)color)
#define sys_sleep() syscall0(syscall_sleep)
#define sys_processCreate(entry) syscall1(syscall_processCreate, (uint32_t)entry)
#define sys_threadAttach(entry, priority) syscall2(syscall_threadAttach, (uint32_t)entry, 0);
#define sys_threadJoin(id) syscall1(syscall_threadJoin, id)

typedef uint32_t (*syscall_callback_t)(cpu_state_t *state, cpu_state_t **returnState);

uint32_t syscall0(syscall_t type);
uint32_t syscall1(syscall_t type, uint32_t arg1);
uint32_t syscall2(syscall_t type, uint32_t arg1, uint32_t arg2);
uint32_t syscall3(syscall_t type, uint32_t arg1, uint32_t arg2, uint32_t arg3);

void sc_mapSyscall(syscall_t syscall, syscall_callback_t callback);
bool sc_init(void *ingored);

#endif
