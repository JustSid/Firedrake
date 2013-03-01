//
//  syscall.h
//  libtest
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

#include "stdint.h"

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
#define SYS_ERRNO         12
#define SYS_TLS_ALLOCATE  13
#define SYS_TLS_FREE      14
#define SYS_TLS_SET       15
#define SYS_TLS_GET       16
#define SYS_PROCESSCREATE 17
#define SYS_PROCESSKILL   18
#define SYS_MMAP          19
#define SYS_MUNMAP        20
#define SYS_MPROTECT      21

uint32_t syscall(int type, ...);

#endif
