//
//  gdt.cpp
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

#include "gdt.h"
#include <machine/memory/memory.h>
#include "cpu.h"
#include <kern/kprintf.h>

void gdt_set_entry(uint64_t *gdt, int16_t index, uint32_t base, uint32_t limit, int32_t flags) __attribute__ ((noinline));
void gdt_set_entry(uint64_t *gdt, int16_t index, uint32_t base, uint32_t limit, int32_t flags)
{
	gdt[index] = limit & INT64_C(0xffff);
	gdt[index] |= (base & INT64_C(0xffffff)) << 16;
	gdt[index] |= (flags & INT64_C(0xff)) << 40;
	gdt[index] |= ((limit >> 16) & INT64_C(0xf)) << 48;
	gdt[index] |= ((flags >> 8 )& INT64_C(0xff)) << 52;
	gdt[index] |= ((base >> 24) & INT64_C(0xff)) << 56;

	__asm__ volatile(""); 
}

void gdt_init(uint64_t *gdt, tss_t *tss)
{
	struct 
	{
		uint16_t limit;
		void *pointer;
	} __attribute__((packed)) gdtp;

	gdtp.limit = (GDT_ENTRIES * sizeof(uint64_t)) - 1;
	gdtp.pointer = gdt;


	gdt_set_entry(gdt, 0, 0x0, 0x0, 0);
	gdt_set_entry(gdt, 1, 0x0, 0xffffffff, GDT_FLAG_SEGMENT | GDT_FLAG_32_BIT | GDT_FLAG_CODESEG | GDT_FLAG_4K | GDT_FLAG_PRESENT);
	gdt_set_entry(gdt, 2, 0x0, 0xffffffff, GDT_FLAG_SEGMENT | GDT_FLAG_32_BIT | GDT_FLAG_DATASEG | GDT_FLAG_4K | GDT_FLAG_PRESENT);
	gdt_set_entry(gdt, 3, 0x0, 0xffffffff, GDT_FLAG_SEGMENT | GDT_FLAG_32_BIT | GDT_FLAG_CODESEG | GDT_FLAG_4K | GDT_FLAG_PRESENT | GDT_FLAG_RING3);
	gdt_set_entry(gdt, 4, 0x0, 0xffffffff, GDT_FLAG_SEGMENT | GDT_FLAG_32_BIT | GDT_FLAG_DATASEG | GDT_FLAG_4K | GDT_FLAG_PRESENT | GDT_FLAG_RING3);

	gdt_set_entry(gdt, 5, reinterpret_cast<uint32_t>(tss), sizeof(tss_t), GDT_FLAG_TSS | GDT_FLAG_PRESENT | GDT_FLAG_RING3);

	__asm__ volatile("lgdt %0" : : "m" (gdtp));
	__asm__ volatile("ljmp %0, $_fakeLabel \n\t _fakeLabel: \n\t" : : "i" (0x8));

	__asm__ volatile("mov $0x10, %ax;");
	__asm__ volatile("mov %ax, %ds;");
	__asm__ volatile("mov %ax, %es;");
	__asm__ volatile("mov %ax, %ss;");

	__asm__ volatile("ltr %%ax" : : "a" (0x28));


	uint32_t *esp = mm::alloc<uint32_t>(vm::get_kernel_directory(), 1, VM_FLAGS_KERNEL);
	tss->esp0 = reinterpret_cast<uint32_t>(esp) + (VM_PAGE_SIZE - sizeof(cpu_state_t));
	tss->ss0  = 0x10;
}
