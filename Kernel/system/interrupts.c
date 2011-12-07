//
//  interrupts.c
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

#include "interrupts.h"
#include "spinlock.h"
#include "port.h"
#include "syslog.h"
#include "assert.h"
#include "panic.h"
#include "string.h"

#define IR_MAX_INTERRUPTS 49 // The number of interrupts we know to handle

static ir_interrupt_handler_t __ir_interruptHandler[IR_MAX_INTERRUPTS];

static bool			__ir_interruptsEnabled = false;
static spinlock_t	__ir_lock = SPINLOCK_INITIALIZER_LOCKED;

// MARK: GDT
#define IR_GDT_FLAG_DATASEG 	0x02
#define IR_GDT_FLAG_CODESEG		0x0A
#define IR_GDT_FLAG_TSS    		0x09

#define IR_GDT_FLAG_SEGMENT		0x10
#define IR_GDT_FLAG_RING0   	0x00
#define IR_GDT_FLAG_RING3   	0x60
#define IR_GDT_FLAG_PRESENT 	0x80

#define IR_GDT_FLAG_4K      	0x800
#define IR_GDT_FLAG_32_BIT  	0x400

#define IR_GDT_ENTRIES 			6

static uint64_t __ir_gdt[IR_GDT_ENTRIES];
static struct tss_t __ir_gdt_tss;

struct tss_t *ir_getTSS()
{
	return &__ir_gdt_tss;
}

static inline void ir_gdt_setEntry(int16_t index, uint32_t base, uint32_t limit, int32_t flags)
{
	__ir_gdt[index] = limit & 0xffffLL;
	__ir_gdt[index] |= (base & 0xffffffLL) << 16;
	__ir_gdt[index] |= (flags & 0xffLL) << 40;
	__ir_gdt[index] |= ((limit >> 16) & 0xfLL) << 48;
	__ir_gdt[index] |= ((flags >> 8 )& 0xffLL) << 52;
	__ir_gdt[index] |= ((base >> 24) & 0xffLL) << 56;
}

void ir_gdt_init()
{
	struct 
	{
		uint16_t limit;
		void *pointer;
	} __attribute__((packed)) gdtp = 
	{
		.limit = IR_GDT_ENTRIES * 8 - 1,
		.pointer = __ir_gdt,
	};


	// Setup the GDT
	ir_gdt_setEntry(0, 0, 0, 0);
	ir_gdt_setEntry(1, 0, 0xFFFFF, IR_GDT_FLAG_SEGMENT | IR_GDT_FLAG_32_BIT | IR_GDT_FLAG_CODESEG | IR_GDT_FLAG_4K | IR_GDT_FLAG_PRESENT);
	ir_gdt_setEntry(2, 0, 0xFFFFF, IR_GDT_FLAG_SEGMENT | IR_GDT_FLAG_32_BIT | IR_GDT_FLAG_DATASEG | IR_GDT_FLAG_4K | IR_GDT_FLAG_PRESENT);
	ir_gdt_setEntry(3, 0, 0xFFFFF, IR_GDT_FLAG_SEGMENT | IR_GDT_FLAG_32_BIT | IR_GDT_FLAG_CODESEG | IR_GDT_FLAG_4K | IR_GDT_FLAG_PRESENT | IR_GDT_FLAG_RING3);
	ir_gdt_setEntry(4, 0, 0xFFFFF, IR_GDT_FLAG_SEGMENT | IR_GDT_FLAG_32_BIT | IR_GDT_FLAG_DATASEG | IR_GDT_FLAG_4K | IR_GDT_FLAG_PRESENT | IR_GDT_FLAG_RING3);

	ir_gdt_setEntry(5, (uint32_t)&__ir_gdt_tss, sizeof(struct tss_t), IR_GDT_FLAG_TSS | IR_GDT_FLAG_PRESENT | IR_GDT_FLAG_RING3);

	// Setup the TSS
	__ir_gdt_tss.back_link 	= 0x0;
	__ir_gdt_tss.esp0		= 0x0;
	__ir_gdt_tss.ss0		= 0x10;

	// Reload the GDT
	__asm__ volatile("lgdt	%0" : : "m" (gdtp));

	// Reload the segment register
	__asm__ volatile("mov	$0x10,  %ax;"
	"mov	%ax,    %ds;"
	"mov	%ax,    %es;"
	"mov	%ax,    %ss;"
	"ljmp	$0x8,   $.1;"
	".1:");

	// TSS
	__asm__ volatile("ltr %%ax" : : "a" (5 << 3));
}

// MARK: IDT
extern void ir_idt_stub_0(void);
extern void ir_idt_stub_1(void);
extern void ir_idt_stub_2(void);
extern void ir_idt_stub_3(void);
extern void ir_idt_stub_4(void);
extern void ir_idt_stub_5(void);
extern void ir_idt_stub_6(void);
extern void ir_idt_stub_7(void);
extern void ir_idt_stub_8(void);
extern void ir_idt_stub_9(void);
extern void ir_idt_stub_10(void);
extern void ir_idt_stub_11(void);
extern void ir_idt_stub_12(void);
extern void ir_idt_stub_13(void);
extern void ir_idt_stub_14(void);
extern void ir_idt_stub_15(void);
extern void ir_idt_stub_16(void);
extern void ir_idt_stub_17(void);
extern void ir_idt_stub_18(void);

extern void ir_idt_stub_32(void);
extern void ir_idt_stub_33(void);

extern void ir_idt_stub_48(void);


#define IR_IDT_FLAG_INTERRUPT_GATE 		0xE
#define IR_IDT_FLAG_PRESENT 			0x80
#define IR_IDT_FLAG_RING0 				0x00
#define IR_IDT_FLAG_RING3				0x60

#define IR_IDT_ENTRIES 					256
static long long unsigned int __ir_idt[IR_IDT_ENTRIES];

static inline void ir_idt_setEntry(uint32_t index, void (*fn)(), uint32_t selector, int flags)
{
	unsigned long int handler = (unsigned long int)fn;
	__ir_idt[index] = handler & 0xffffLL;
	__ir_idt[index] |= (selector & 0xffffLL) << 16;
	__ir_idt[index] |= (flags & 0xffLL) << 40;
	__ir_idt[index] |= ((handler>> 16) & 0xffffLL) << 48;
}

void ir_idt_init()
{
	struct 
	{
		unsigned short int limit;
		void *pointer;
	} __attribute__((packed)) idtp = 
	{
		.limit = IR_IDT_ENTRIES * 8 - 1,
		.pointer = __ir_idt,
	};

	// Excpetion-Handler
	ir_idt_setEntry(0, ir_idt_stub_0, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(1, ir_idt_stub_1, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(2, ir_idt_stub_2, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(3, ir_idt_stub_3, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(4, ir_idt_stub_4, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(5, ir_idt_stub_5, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(6, ir_idt_stub_6, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(7, ir_idt_stub_7, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(8, ir_idt_stub_8, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(9, ir_idt_stub_9, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(10, ir_idt_stub_10, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(11, ir_idt_stub_11, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(12, ir_idt_stub_12, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(13, ir_idt_stub_13, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(14, ir_idt_stub_14, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(15, ir_idt_stub_15, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(16, ir_idt_stub_16, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(17, ir_idt_stub_17, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(18, ir_idt_stub_18, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);

	// IRQ-Handler
	ir_idt_setEntry(32, ir_idt_stub_32, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);
	ir_idt_setEntry(33, ir_idt_stub_33, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING0 | IR_IDT_FLAG_PRESENT);

	// Syscall
	ir_idt_setEntry(48, ir_idt_stub_48, 0x8, IR_IDT_FLAG_INTERRUPT_GATE | IR_IDT_FLAG_RING3 | IR_IDT_FLAG_PRESENT);

	__asm__ volatile("lidt %0" : : "m" (idtp));
}

// MARK: PIC
void ir_pic_init()
{
	// Initialize the master pic
	outb(0x20, 0x11);
	outb(0x21, 0x20);
	outb(0x21, 0x04);
	outb(0x21, 0x01);

	// Initialize the slave PIC
	outb(0xA0, 0x11);
	outb(0xA1, 0x28);
	outb(0xA1, 0x02);
	outb(0xA1, 0x01);

	// Demask the interrupts
	outb(0x20, 0x0);
	outb(0xA0, 0x0);
}


// MARK: Interrupt processing
void ir_enableInterrupts()
{	
	spinlock_lock(&__ir_lock);

	if(!__ir_interruptsEnabled)
	{
		__ir_interruptsEnabled = true;

		__asm__ volatile("sti");
		outb(0x70, inb(0x70) & 0x7F);
	}

	spinlock_unlock(&__ir_lock);
}

void ir_disableInterrupts()
{
	spinlock_lock(&__ir_lock);

	if(__ir_interruptsEnabled)
	{
		__ir_interruptsEnabled = false;

		__asm__ volatile("cli");
		outb(0x70, inb(0x70) | 0x80);
	}

	spinlock_unlock(&__ir_lock);
}

// MARK: Interrupt handler handling
void ir_setInterruptHandler(ir_interrupt_handler_t handler, uint32_t interrupt)
{
	assert(handler && interrupt < IR_MAX_INTERRUPTS);
	__ir_interruptHandler[interrupt] = handler;
}

cpu_state_t *ir_handleInterrupt(cpu_state_t *state)
{
	ir_interrupt_handler_t handler = __ir_interruptHandler[state->interrupt];
	if(handler)
	{
		cpu_state_t *result = handler(state);

		if(result)
			return result;
	}

	panic("Unhandled interrupt %i occured!", state->interrupt);
	return state;
}

// MARK: Default handlers
cpu_state_t *__ir_defaultHandler(cpu_state_t *state)
{
	if(state->interrupt >= 0x28)
	outb(0xA0, 0x20);

	outb(0x20, 0x20);
	return state;
}

extern cpu_state_t *__ir_exceptionHandler(cpu_state_t *state); // exceptions.c

bool ir_init(void *ignored)
{
	memset(__ir_interruptHandler, 0, IR_MAX_INTERRUPTS * sizeof(ir_interrupt_handler_t));

	// Set the default handlers
	for(int i=0x0; i<0x2F; i++)
	{
		if(i >= 0x0 && i < 0x12)
		ir_setInterruptHandler(__ir_exceptionHandler, i);

		if(i >= 0x20 && i < 0x2F)
		ir_setInterruptHandler(__ir_defaultHandler, i);
	}


	ir_pic_init();
	ir_gdt_init(); // Setup the GDT
	ir_idt_init(); // Setup the IDT

	spinlock_unlock(&__ir_lock);

	return true;
}
