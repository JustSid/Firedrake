//
//  idt.S
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

.global _kernel_page_directory
.global ir_handl_interrupt

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

INTERRUPT(0x30)
INTERRUPT(0x31)
INTERRUPT(0x32)
INTERRUPT(0x33)
INTERRUPT(0x34)
INTERRUPT(0x35)
INTERRUPT(0x36)
INTERRUPT(0x37)
INTERRUPT(0x38)

INTERRUPT(0x80)

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
	movl (_kernel_page_directory), %ecx
	movl %cr3, %eax

	// Load the kernels page directory if it isn't active yet
	cmpl %eax, %ecx
	jz idt_entry_handler_call_handler

	movl %ecx, %cr3

idt_entry_handler_call_handler:
	// Call the high level interrupt handler
	pushl %esp
	call ir_handle_interrupt
	addl $4, %esp

	// Load the new stack
	movl %eax, %esp

	// Get the address of the new page directory
	movl %fs, %eax
	imull $0x89c, %eax // 0x89c == sizeof(ir::trampoline_data_t)

	movl $0xffb00000, %ecx
	addl %eax, %ecx

	movl (%ecx), %ecx

	// Load the new page directory if it is NOT the kernels page directory
	// This potentially saves trashing the TLB a few times
	cmpl (_kernel_page_directory), %ecx
	jz idt_entry_handler_outro

	movl %ecx, %cr3

idt_entry_handler_outro:
	popl %gs
	popl %fs
	popl %es
	popl %ds
	popa

	addl $8, %esp
	iret

GLOBAL(idt_end)
