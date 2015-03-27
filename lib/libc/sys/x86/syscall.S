//
//  syscall.S
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

#include "../asm.h"

#ifndef __KERNEL

TEXT()
ENTRY(__syscall)
	pushl %ebp
	movl %esp, %ebp

	pushl %edi
	pushl %esi
	pushl %ebx

	movl 0x8(%ebp), %eax

	movl 0xc(%ebp), %ecx
	movl 0x10(%ebp), %edi
	movl 0x14(%ebp), %esi
	movl 0x18(%ebp), %edx
	movl 0x1c(%ebp), %ebx

	int  $0x80

	jecxz 1f

	pushl %eax
	pushl %ecx
	call __tls_setErrno
	add $0x8, %esp

1:
	popl %ebx
	popl %esi
	popl %edi

	popl %ebp
	ret

ENTRY(__kern_trap)
	pushl %ebp
	movl %esp, %ebp

	pushl %edi
	pushl %esi
	pushl %ebx

	movl 0x8(%ebp), %eax

	movl 0xc(%ebp), %ecx
	movl 0x10(%ebp), %edi
	movl 0x14(%ebp), %esi
	movl 0x18(%ebp), %edx
	movl 0x1c(%ebp), %ebx

	int  $0x81

	popl %ebx
	popl %esi
	popl %edi

	popl %ebp
	ret

#endif
