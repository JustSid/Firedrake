//
//  smp_bootstrap.S
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

#include <asm.h>

.global smp_rendezvous_point

TEXT()
ENTRY(smp_bootstrap_begin)
	movl $0x10, %eax
	movl %eax, %ss
	movl %eax, %ds
	movl %eax, %es

	movl $smp_bootstrap_finish, %eax
	jmp *%eax

ENTRY(smp_bootstrap_finish)
	movl $smp_stack_top, %esp
	
	// Push an empty stack frame
	pushl $0
	pushl $0

	call smp_rendezvous_point

GLOBAL(smp_bootstrap_end)

.bss

.global smp_stack_bottom
.global smp_stack_top

smp_stack_bottom:
.space 4096
smp_stack_top:
