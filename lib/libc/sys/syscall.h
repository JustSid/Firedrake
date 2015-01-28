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

/* Syscall */
#define SYS_Exit  0
#define SYS_Open  1
#define SYS_Close 2
#define SYS_Read  3
#define SYS_Write 4
#define SYS_Seek  5
#define SYS_Stat  6
#define SYS_Pid   7

#define SYS_Mmap     40
#define SYS_Munmap   41
#define SYS_Mprotect 42

/* Message Trap */
#define SYS_IPC_TaskPort   128
#define SYS_IPC_ThreadPort 129
#define SYS_IPC_Message    130

#define __SYS_MaxSyscalls 256

#ifndef __KERNEL
unsigned int syscall(int type, ...);
unsigned int kern_trap(int type, ...);
#endif /* __KERNEL */

__END_DECLS


#endif /* _SYS_SYSCALL_H_ */
