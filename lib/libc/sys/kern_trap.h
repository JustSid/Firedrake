//
//  kern_trap.h
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2015 by Sidney Just
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

#ifndef _SYS_KERNTRAP_H_
#define _SYS_KERNTRAP_H_

#include "cdefs.h"

__BEGIN_DECLS

#define KERN_IPC_TaskPort   0
#define KERN_IPC_ThreadPort 1
#define KERN_IPC_Message    2
#define KERN_IPC_AllocatePort 3
#define KERN_IPC_GetSpecialPort 4
#define KERN_IPC_DeallocatePort 5

unsigned int __kern_trap(int type, ...);

#define KERN_TRAP8(type, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
	__kern_trap(type, (arg0), (arg1), (arg2), (arg3), (arg4), (arg5), (arg6), (arg7))

#define KERN_TRAP7(type, arg0, arg1, arg2, arg3, arg4, arg5, arg6) \
	KERN_TRAP8(type, arg0, arg1, arg2, arg3, arg4, arg5, arg6, 0)

#define KERN_TRAP6(type, arg0, arg1, arg2, arg3, arg4, arg5) \
	KERN_TRAP8(type, arg0, arg1, arg2, arg3, arg4, arg5, 0, 0)

#define KERN_TRAP5(type, arg0, arg1, arg2, arg3, arg4) \
	KERN_TRAP8(type, arg0, arg1, arg2, arg3, arg4, 0, 0, 0)

#define KERN_TRAP4(type, arg0, arg1, arg2, arg3) \
	KERN_TRAP8(type, arg0, arg1, arg2, arg3, 0, 0, 0, 0)

#define KERN_TRAP3(type, arg0, arg1, arg2) \
	KERN_TRAP8(type, arg0, arg1, arg2, 0, 0, 0, 0, 0)

#define KERN_TRAP2(type, arg0, arg1) \
	KERN_TRAP8(type, arg0, arg1, 0, 0, 0, 0, 0, 0)

#define KERN_TRAP1(type, arg0) \
	KERN_TRAP8(type, arg0, 0, 0, 0, 0, 0, 0, 0)

#define KERN_TRAP0(type) \
	KERN_TRAP8(type, 0, 0, 0, 0, 0, 0, 0, 0)

__END_DECLS

#endif /* _SYS_KERNTRAP_H_ */
