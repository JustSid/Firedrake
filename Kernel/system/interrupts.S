//
//  interrupts.S
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

.extern ir_handleInterrupt // The high-level interrupt handler

.macro ir_idt_stub nr
.global ir_idt_stub_\nr
ir_idt_stub_\nr:
 pushl $0 // Push a dummy value onto the stack
 pushl $\nr
 jmp ir_interruptHandler 
.endm

.macro ir_idt_stub_error_code nr
.global ir_idt_stub_\nr
ir_idt_stub_\nr:
 pushl $\nr
 jmp ir_interruptHandler
.endm

ir_idt_stub 0
ir_idt_stub 1
ir_idt_stub 2
ir_idt_stub 3
ir_idt_stub 4
ir_idt_stub 5
ir_idt_stub 6
ir_idt_stub 7
ir_idt_stub_error_code 8
ir_idt_stub 9
ir_idt_stub_error_code 10
ir_idt_stub_error_code 11
ir_idt_stub_error_code 12
ir_idt_stub_error_code 13
ir_idt_stub_error_code 14
ir_idt_stub 15
ir_idt_stub 16
ir_idt_stub_error_code 17
ir_idt_stub 18

ir_idt_stub 32 // Timer
ir_idt_stub 33 // PS/2 1
ir_idt_stub 34 // Cascade
ir_idt_stub 35 // RS-232
ir_idt_stub 36 // RS 232
ir_idt_stub 37 // LPT 2
ir_idt_stub 38 // Floppy 
ir_idt_stub 39 // LPT 1
ir_idt_stub 40 // Real Time Clock
ir_idt_stub 41 // 
ir_idt_stub 42 // IDE/ATAPI/ATA
ir_idt_stub 43 // IDE/ATAPI/ATA
ir_idt_stub 44 // PS/2 2
ir_idt_stub 45 // FPU
ir_idt_stub 46 // primary IDE/ATAPI/ATA
ir_idt_stub 47 // secondary IDE/ATAPI/ATA

ir_idt_stub 48 // Syscall

ir_interruptHandler:
	push %ebp
	push %edi
	push %esi
	push %edx
	push %ecx
	push %ebx
	push %eax

	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es

	push %esp
	call ir_handleInterrupt
	mov %eax, %esp

	mov $0x23, %ax
	mov %ax, %ds
	mov %ax, %es

	pop %eax
	pop %ebx
	pop %ecx
	pop %edx
	pop %esi
	pop %edi
	pop %ebp

	add $8, %esp
	iret
