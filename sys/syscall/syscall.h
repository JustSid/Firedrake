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
#include <system/cpu.h>

#include "errno.h"

#define _SYS_MAXCALLS 		128 // Increase if needed

#define SYS_PRINT 			0 // syscall1(char *message)
#define SYS_PRINTCOLOR 		1 // syscall2(int color, char *message)
#define SYS_EXIT 			2 // syscall0()
#define SYS_SLEEP			3 // syscall0()
#define SYS_THREADATTACH	4 // syscall4(void *function, void *arg1, unsigned int stacksize, void *arg2), returns thread id, stacksize must be a multiple of 4k
#define SYS_THREADEXIT		5 // syscall0()
#define SYS_THREADJOIN		6 // syscall1(unsigned int tid)
#define SYS_PROCESSCREATE	7 // syscall1(char *name), returns pid
#define SYS_PROCESSKILL		8 // syscall1(unsgined int pid)
#define SYS_MMAP			9 // void *mmap(void *addr, size_t length, int prot, int flags, int fd, uint32_t offset)
#define SYS_UNMAP			10 // (void*, size_t) // Doesn't exist yet!
#define SYS_FORK			11 // uint32_t fork(void)

typedef uint32_t (*syscall_callback_t)(uint32_t *esp, uint32_t *uesp, int *errno);

void sc_setSyscallHandler(uint32_t syscall, syscall_callback_t callback);
bool sc_init(void *ingored);

#endif
