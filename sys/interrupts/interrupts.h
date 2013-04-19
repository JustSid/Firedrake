//
//  interrupts.h
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

#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

#include <prefix.h>
#include <system/cpu.h>

#define IDT_FLAG_INTERRUPT_GATE 	0xE
#define IDT_FLAG_PRESENT 			0x80
#define IDT_FLAG_RING0 				0x00
#define IDT_FLAG_RING3				0x60

#define IDT_ENTRIES 				256

// exceptions
extern void idt_exception_divbyzero(); // 0
extern void idt_exception_debug(); // 1
extern void idt_exception_breakpoint(); // 3
extern void idt_exception_overflow(); // 4
extern void idt_exception_boundrange(); // 5
extern void idt_exception_opcode(); // 6
extern void idt_exception_devicenotavailable(); // 7
extern void idt_exception_doublefault(); // 8 error
extern void idt_exception_segmentoverrun(); // 9
extern void idt_exception_invalidtss(); // 10 error
extern void idt_exception_segmentnotpresent(); // 11 error
extern void idt_exception_stackfault(); // 12 error
extern void idt_exception_protectionfault(); // 13 error
extern void idt_exception_pagefault(); // 14 error
extern void idt_exception_fpuerror(); // 16
extern void idt_exception_alignment(); // 17 error
extern void idt_exception_machinecheck(); // 18
extern void idt_exception_simd(); // 19

// interrupts
extern void idt_interrupt_0x02(); // NMI
extern void idt_interrupt_0x20();
extern void idt_interrupt_0x21();
extern void idt_interrupt_0x22();
extern void idt_interrupt_0x23();
extern void idt_interrupt_0x24();
extern void idt_interrupt_0x25();
extern void idt_interrupt_0x26();
extern void idt_interrupt_0x27();
extern void idt_interrupt_0x28();
extern void idt_interrupt_0x29();
extern void idt_interrupt_0x2A();
extern void idt_interrupt_0x2B();
extern void idt_interrupt_0x2C();
extern void idt_interrupt_0x2D();
extern void idt_interrupt_0x2E();
extern void idt_interrupt_0x2F();

extern void idt_interrupt_0x31(); // Forced reshedule
extern void idt_interrupt_0x80(); // Syscall

// Interrupts that are free to use (ie have no special purpose)
extern void idt_interrupt_0x81();
extern void idt_interrupt_0x82();
extern void idt_interrupt_0x83();
extern void idt_interrupt_0x84();
extern void idt_interrupt_0x85();
extern void idt_interrupt_0x86();
extern void idt_interrupt_0x87();
extern void idt_interrupt_0x88();
extern void idt_interrupt_0x89();
extern void idt_interrupt_0x8A();
extern void idt_interrupt_0x8B();
extern void idt_interrupt_0x8C();
extern void idt_interrupt_0x8D();
extern void idt_interrupt_0x8E();
extern void idt_interrupt_0x8F();
extern void idt_interrupt_0x90();
extern void idt_interrupt_0x91();
extern void idt_interrupt_0x92();
extern void idt_interrupt_0x93();
extern void idt_interrupt_0x94();
extern void idt_interrupt_0x95();
extern void idt_interrupt_0x96();
extern void idt_interrupt_0x97();
extern void idt_interrupt_0x98();
extern void idt_interrupt_0x99();
extern void idt_interrupt_0x9A();
extern void idt_interrupt_0x9B();
extern void idt_interrupt_0x9C();
extern void idt_interrupt_0x9D();
extern void idt_interrupt_0x9E();
extern void idt_interrupt_0x9F();

typedef uint32_t (*ir_interrupt_handler_t)(uint32_t);
typedef void (*ir_interrupt_callback_t)(cpu_state_t *);

bool ir_isValidInterrupt(uint32_t interrupt, bool publicOnly);
uint32_t ir_interruptPublicBegin();
uint32_t ir_interruptPublicEnd();

void ir_disableInterrupts(bool disableNMI);
void ir_enableInterrupts(bool enableNMI);

void ir_setInterruptHandler(ir_interrupt_handler_t handler, uint32_t interrupt);
void ir_setInterruptCallback(ir_interrupt_callback_t callback, uint32_t interrupt);

bool ir_isInsideInterruptHandler();
uint32_t ir_lastInterruptESP();

void ir_idt_init(uint64_t *idt, uint32_t offset);
bool ir_init(void *unused);

#endif /* _INTERRUPTS_H_ */
