//
//  lock.S
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

#include <asm.h>
#include <config.h>

.global kern_dumpSpinlock
.global spinlock_wait

ENTRY(spinlock_lock)
	pushl %edi
	movl 0x8(%esp), %edi
	movb $0x1, %cl

	jmp spinlock_tryObtain
spinlock_wait:
	// Spinlock code is exectued before interrupt handler are in place. Once they are in place, the nop's will be replaced by int $0x31 (0xcd 0x31) 
	nop
	nop

spinlock_tryObtain:
	xorb %al, %al
	lock cmpxchgb %cl, (%edi)
	jne spinlock_wait

	popl %edi
	ret


ENTRY(spinlock_tryLock)
	pushl %edi
	movl 0x8(%esp), %edi
	movb $0x1, %cl
	xorb %al, %al
	lock cmpxchgb %cl, (%edi)
	jne spinlock_failedObtain
	movl $0x1, %eax
	popl %edi
	ret

spinlock_failedObtain:
	xorl %eax, %eax
	popl %edi
	ret


ENTRY(spinlock_unlock)
	movl 0x4(%esp), %eax
	movb $0x0, (%eax)
	ret
	