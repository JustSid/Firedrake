//
//  idt.h
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

#ifndef _IDT_H_
#define _IDT_H_

#define IR_TRAMPOLINE_BEGIN          0xffaff000
#define IR_TRAMPOLINE_PAGES          2
#define IR_TRAMPOLINE_PAGEDIR_OFFSET 0x1000

#define IDT_FLAG_TASK_GATE      0x5
#define IDT_FLAG_INTERRUPT_GATE 0xe
#define IDT_FLAG_TRAP_GATE      0xf

#define IDT_FLAG_PRESENT        0x80
#define IDT_FLAG_RING0          0x00
#define IDT_FLAG_RING3          0x60

#define IDT_ENTRIES 256

#ifndef __IDT_S__

BEGIN_EXTERNC
void idt_exception_divbyzero(); // 0
void idt_exception_debug(); // 1
void idt_exception_breakpoint(); // 3
void idt_exception_overflow(); // 4
void idt_exception_boundrange(); // 5
void idt_exception_opcode(); // 6
void idt_exception_devicenotavailable(); // 7
void idt_exception_doublefault(); // 8 error
void idt_exception_segmentoverrun(); // 9
void idt_exception_invalidtss(); // 10 error
void idt_exception_segmentnotpresent(); // 11 error
void idt_exception_stackfault(); // 12 error
void idt_exception_protectionfault(); // 13 error
void idt_exception_pagefault(); // 14 error
void idt_exception_fpuerror(); // 16
void idt_exception_alignment(); // 17 error
void idt_exception_machinecheck(); // 18
void idt_exception_simd(); // 19

// Devices
void idt_interrupt_0x02(); // NMI
void idt_interrupt_0x20();
void idt_interrupt_0x21();
void idt_interrupt_0x22();
void idt_interrupt_0x23();
void idt_interrupt_0x24();
void idt_interrupt_0x25();
void idt_interrupt_0x26();
void idt_interrupt_0x27();
void idt_interrupt_0x28();
void idt_interrupt_0x29();
void idt_interrupt_0x2a();
void idt_interrupt_0x2b();
void idt_interrupt_0x2c();
void idt_interrupt_0x2d();
void idt_interrupt_0x2e();
void idt_interrupt_0x2f();

// APIC
void idt_interrupt_0x30(); // Performance counter
void idt_interrupt_0x31(); // CMCI
void idt_interrupt_0x32(); // Spurious
void idt_interrupt_0x33(); // Error
void idt_interrupt_0x34(); // Thermal
void idt_interrupt_0x35(); // Timer
void idt_interrupt_0x36(); // IPI
void idt_interrupt_0x37(); // LINT0
void idt_interrupt_0x38(); // LINT1

// System call
void idt_interrupt_0x80();

// General purpose interrupts
#define idt_interrupt_set(n) \
	void idt_interrupt_## n ## 0(); \
	void idt_interrupt_## n ## 1(); \
	void idt_interrupt_## n ## 2(); \
	void idt_interrupt_## n ## 3(); \
	void idt_interrupt_## n ## 4(); \
	void idt_interrupt_## n ## 5(); \
	void idt_interrupt_## n ## 6(); \
	void idt_interrupt_## n ## 7(); \
	void idt_interrupt_## n ## 8(); \
	void idt_interrupt_## n ## 9(); \
	void idt_interrupt_## n ## a(); \
	void idt_interrupt_## n ## b(); \
	void idt_interrupt_## n ## c(); \
	void idt_interrupt_## n ## d(); \
	void idt_interrupt_## n ## e(); \
	void idt_interrupt_## n ## f()
	
idt_interrupt_set(0x9);
idt_interrupt_set(0xa);
idt_interrupt_set(0xb);
idt_interrupt_set(0xc);
idt_interrupt_set(0xd);
idt_interrupt_set(0xe);
idt_interrupt_set(0xf);

END_EXTERNC

namespace ir
{
	void idt_init(uint64_t *idt, uint32_t offset);
}

#endif /* __IDT_S__ */

#endif /* _IDT_H_ */
