//
//  syscall.h
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

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <prefix.h>
#include <errno.h>
#include <system/cpu.h>

#define _SYS_MAXCALLS 128 // Increase if needed

#define SYS_PRINT         0
#define SYS_EXIT          1
#define SYS_PID           2
#define SYS_PPID          3
#define SYS_FORK          4
#define SYS_WAIT          5
#define SYS_THREADYIELD   6
#define SYS_THREADSLEEP   7
#define SYS_THREADATTACH  8
#define SYS_THREADEXIT    9
#define SYS_THREADJOIN    10
#define SYS_THREADSELF    11
#define SYS_TLS_AREA      12
#define SYS_PROCESSCREATE 17
#define SYS_PROCESSKILL   18
#define SYS_MMAP          19
#define SYS_MUNMAP        20
#define SYS_MPROTECT      21

void *sc_mapProcessMemory(void *memory, vm_address_t *mappedBase, size_t pages, int *errno);

typedef uint32_t (*syscall_callback_t)(uint32_t *esp, uint32_t *uesp, int *errno);

void sc_setSyscallHandler(uint32_t syscall, syscall_callback_t callback);
bool sc_init(void *ingored);

#endif
