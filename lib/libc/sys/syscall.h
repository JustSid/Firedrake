//
//  syscall.h
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

#ifndef _SYS_SYSCALL_H_
#define _SYS_SYSCALL_H_

#include "cdefs.h"

__BEGIN_DECLS

#define SYS_Exit  0
#define SYS_Open  1
#define SYS_Close 2
#define SYS_Read  3
#define SYS_Write 4
#define SYS_Seek  5
#define SYS_Stat  6
#define SYS_Pid   7
#define SYS_Ioctl 8

#define SYS_ThreadCreate 10
#define SYS_ThreadJoin   11
#define SYS_ThreadYield  12
#define SYS_Fork         13
#define SYS_Exec         14
#define SYS_Spawn        15

#define SYS_Mmap     20
#define SYS_Munmap   21
#define SYS_Mprotect 22
#define SYS_Msync    23

unsigned int __syscall(int type, ...);

#define SYSCALL8(type, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
	__syscall(type, (arg0), (arg1), (arg2), (arg3), (arg4), (arg5), (arg6), (arg7))

#define SYSCALL7(type, arg0, arg1, arg2, arg3, arg4, arg5, arg6) \
	SYSCALL8(type, arg0, arg1, arg2, arg3, arg4, arg5, arg6, 0)

#define SYSCALL6(type, arg0, arg1, arg2, arg3, arg4, arg5) \
	SYSCALL8(type, arg0, arg1, arg2, arg3, arg4, arg5, 0, 0)

#define SYSCALL5(type, arg0, arg1, arg2, arg3, arg4) \
	SYSCALL8(type, arg0, arg1, arg2, arg3, arg4, 0, 0, 0)

#define SYSCALL4(type, arg0, arg1, arg2, arg3) \
	SYSCALL8(type, arg0, arg1, arg2, arg3, 0, 0, 0, 0)

#define SYSCALL3(type, arg0, arg1, arg2) \
	SYSCALL8(type, arg0, arg1, arg2, 0, 0, 0, 0, 0)

#define SYSCALL2(type, arg0, arg1) \
	SYSCALL8(type, arg0, arg1, 0, 0, 0, 0, 0, 0)

#define SYSCALL1(type, arg0) \
	SYSCALL8(type, arg0, 0, 0, 0, 0, 0, 0, 0)

#define SYSCALL0(type) \
	SYSCALL8(type, 0, 0, 0, 0, 0, 0, 0, 0)

__END_DECLS

#endif /* _SYS_SYSCALL_H_ */
