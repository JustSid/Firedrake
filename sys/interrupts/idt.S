//
//  idt.S
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

#define __kasm__
#include <asm.h>
#include "trampoline.h"
#undef __kasm__

.text
.align 4096

.global idt_sectionBegin
idt_sectionBegin:

#define IDT_ENTRY(n, name) 		\
ENTRY(name)						\
	pushl	$0; 				\
	pushl	$n;					\
	jmp		idt_entry_handler


#define IDT_ENTRY_ERROR(n, name)	\
ENTRY(name)							\
	pushl	$n; 					\
	jmp		idt_entry_handler


#define INTERRUPT(n) \
	IDT_ENTRY(n, idt_interrupt_ ## n)


#define EXCEPTION(n, name) \
	IDT_ENTRY(n, idt_exception_ ## name)

#define EXCEPTION_ERROR(n, name) \
	IDT_ENTRY_ERROR(n, idt_exception_ ## name)


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
INTERRUPT(0x27)
INTERRUPT(0x28)
INTERRUPT(0x29)
INTERRUPT(0x2A)
INTERRUPT(0x2B)
INTERRUPT(0x2C)
INTERRUPT(0x2D)
INTERRUPT(0x2E)
INTERRUPT(0x2F)
INTERRUPT(0x30)
INTERRUPT(0x31) // Force reschedule without time update
INTERRUPT(0x80)

INTERRUPT(0x81)
INTERRUPT(0x82)
INTERRUPT(0x83)
INTERRUPT(0x84)
INTERRUPT(0x85)
INTERRUPT(0x86)
INTERRUPT(0x87)
INTERRUPT(0x88)
INTERRUPT(0x89)
INTERRUPT(0x8A)
INTERRUPT(0x8B)
INTERRUPT(0x8C)
INTERRUPT(0x8D)
INTERRUPT(0x8E)
INTERRUPT(0x8F)
INTERRUPT(0x90)
INTERRUPT(0x91)
INTERRUPT(0x92)
INTERRUPT(0x93)
INTERRUPT(0x94)
INTERRUPT(0x95)
INTERRUPT(0x96)
INTERRUPT(0x97)
INTERRUPT(0x98)
INTERRUPT(0x99)
INTERRUPT(0x9A)
INTERRUPT(0x9B)
INTERRUPT(0x9C)
INTERRUPT(0x9D)
INTERRUPT(0x9E)
INTERRUPT(0x9F)

ENTRY(idt_entry_handler)
	pusha
	pushl %ds
	pushl %es
	pushl %fs
	pushl %gs

	// Load the kernels segment register
	movl $0x10, %eax
	movl %eax, %ds
	movl %eax, %es

	cld

	// Load the kernel page directory if needed, because this process involves trashing the TLB we first check wether its actually needed to switch the address space
	movl $0x1000, %ecx
	movl %cr3, %eax

	cmpl %eax, %ecx
	jz idt_entry_handler_call_handler

	movl %ecx, %cr3

idt_entry_handler_call_handler:
	pushl %esp
	call ir_handleInterrupt
	addl $4, %esp

	// Load the new stack returned by ir_handleInterrupt
	movl %eax, %esp 

	// Switch to a new page directory if needed
	// Note that at this point the active page directory is always the kernel ones, so we can safely compare it with a scalar type
	movl (IR_TRAMPOLINE_PAGEDIR), %ecx
	cmpl $0x1000, %ecx
	jz idt_entry_handler_outro

	movl %ecx, %cr3

idt_entry_handler_outro:
	popl %gs
	popl %fs
	popl %es
	popl %ds
	popa

	addl $8, %esp // Remove the error code and interrupt number from the stack
	iret

.global idt_sectionEnd
idt_sectionEnd:
