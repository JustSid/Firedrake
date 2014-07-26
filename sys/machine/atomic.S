//
//  atomic.S
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

#include <libc/sys/asm.h>


ENTRY(atomic_compare_swap_i32)
ENTRY(atomic_compare_swap_u32)
	movl 4(%esp), %eax
	movl 8(%esp), %edx
	movl 12(%esp), %ecx

	lock cmpxchgl %edx, 0(%ecx)

	sete %al
	movzbl %al, %eax
	ret

ENTRY(atomic_add_i32)
ENTRY(atomic_add_u32)
	movl 4(%esp), %eax
	movl 8(%esp), %ecx

	lock xaddl %eax, 0(%ecx)
	ret
	

ENTRY(atomic_compare_swap_i64)
ENTRY(atomic_compare_swap_u64)
	pushl %edi
	pushl %ebx

	movl 12(%esp), %eax
	movl 16(%esp), %edx
	movl 20(%esp), %ebx
	movl 24(%esp), %ecx
	movl 28(%esp), %edi

	lock cmpxchg8b (%edi)

	sete   %al
	movzbl %al, %eax

	popl %ebx
	popl %edi
	ret

ENTRY(atomic_add_i64)
ENTRY(atomic_add_u64)
	pushl %edi
	pushl %ebx

	movl 20(%esp), %edi
	movl 0(%edi), %eax
	movl 4(%edi), %ecx

1:
	movl %eax, %ebx
	movl %edx, %ecx

	addl 12(%esp), %ebx
	addl 16(%esp), %ebx

	lock cmpxchg8b (%edi)
	jnz 1b

	popl %ebx
	popl %edi
	ret