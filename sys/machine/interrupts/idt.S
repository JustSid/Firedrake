//
//  idt.S
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

TEXT()
GLOBAL(idt_begin)

#define IDT_ENTRY(n, name) \
	ENTRY(name) \
		pushl $0; \
		pushl $n; \
		jmp idt_entry_handler;

#define IDT_ENTRY_ERROR(n, name) \
	ENTRY(name) \
		pushl $n; \
		jmp idt_entry_handler;

#define INTERRUPT(n) \
	IDT_ENTRY(n, idt_interrupt_ ## n)

#define EXCEPTION(n, name) \
	IDT_ENTRY(n, idt_exception_ ## name)

#define EXCEPTION_ERROR(n, name) \
	IDT_ENTRY_ERROR(n, idt_exception_ ## name)

#define INTERRUPT_SET(n) \
	INTERRUPT(## n ## 0) \
	INTERRUPT(## n ## 1) \
	INTERRUPT(## n ## 2) \
	INTERRUPT(## n ## 3) \
	INTERRUPT(## n ## 4) \
	INTERRUPT(## n ## 5) \
	INTERRUPT(## n ## 6) \
	INTERRUPT(## n ## 7) \
	INTERRUPT(## n ## 8) \
	INTERRUPT(## n ## 9) \
	INTERRUPT(## n ## a) \
	INTERRUPT(## n ## b) \
	INTERRUPT(## n ## c) \
	INTERRUPT(## n ## d) \
	INTERRUPT(## n ## e) \
	INTERRUPT(## n ## f)

EXCEPTION(0, divbyzero)
EXCEPTION(1, debug)
EXCEPTION(3, breakpoint)
EXCEPTION(4, overflow)
EXCEPTION(5, boundrange)
EXCEPTION(6, opcode)
EXCEPTION(7, devicenotavailable)
EXCEPTION_ERROR(8, doublefault)
EXCEPTION(9, segmentoverrun)
EXCEPTION_ERROR(10, invalidtss)
EXCEPTION_ERROR(11, segmentnotpresent)
EXCEPTION_ERROR(12, stackfault)
EXCEPTION_ERROR(13, protectionfault)
EXCEPTION_ERROR(14, pagefault)
EXCEPTION(16, fpuerror)
EXCEPTION_ERROR(17, alignment)
EXCEPTION(18, machinecheck)
EXCEPTION(19, simd)

INTERRUPT(0x02)
INTERRUPT(0x20)
INTERRUPT(0x21)
INTERRUPT(0x22)
INTERRUPT(0x23)
INTERRUPT(0x24)
INTERRUPT(0x25)
INTERRUPT(0x26)
INTERRUPT(0x27)
INTERRUPT(0x28)
INTERRUPT(0x29)
INTERRUPT(0x2a)
INTERRUPT(0x2b)
INTERRUPT(0x2c)
INTERRUPT(0x2d)
INTERRUPT(0x2e)
INTERRUPT(0x2f)

INTERRUPT_SET(0x3)
INTERRUPT_SET(0x4)
INTERRUPT_SET(0x5)
INTERRUPT_SET(0x6)
INTERRUPT_SET(0x7)
INTERRUPT_SET(0x8)
INTERRUPT_SET(0x9)
INTERRUPT_SET(0xa)
INTERRUPT_SET(0xb)
INTERRUPT_SET(0xc)
INTERRUPT_SET(0xd)
INTERRUPT_SET(0xe)
INTERRUPT_SET(0xf)

ENTRY(idt_entry_handler)
	pusha
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs

	// Switch to the kernel segments (ss is already set upon entry)
	movl $0x10, %eax
	movl %eax, %ds
	movl %eax, %es

	cld

	// Load the current page directory and the kernel ones into eax and ecx respectively
	movl $0xffb00000, %ecx
	movl (%ecx), %ecx
	movl %cr3, %eax

	// Load the kernels page directory if it isn't active yet
	cmpl %eax, %ecx
	jz 1f

	movl %ecx, %cr3

1:
	// Call the high level interrupt handler
	pushl %esp
	movl $ir_handle_interrupt, %eax
	call *%eax
	addl $4, %esp

	// Load the new stack
	movl %eax, %esp

	// Get the address of the new page directory
	movl %fs:0, %eax
	imull $0x89c, %eax // 0x89c == sizeof(Sys::Trampoline)

	movl $0xffb01000, %ecx
	addl %eax, %ecx

	movl (%ecx), %ecx

	// Load the new page directory if it is NOT the kernels page directory
	// This potentially saves trashing the TLB a few times
	movl $0xffb00000, %eax
	movl (%eax), %eax
	cmpl %eax, %ecx
	jz 1f

	movl %ecx, %cr3

1:
	popl %gs
	popl %fs
	popl %es
	popl %ds
	popa

	addl $8, %esp
	iret

GLOBAL(idt_end)
