//
//  gdt.cpp
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

#include "gdt.h"
#include "cpu.h"
#include <machine/memory/memory.h>
#include <kern/kprintf.h>
#include <machine/interrupts/trampoline.h>

namespace Sys
{
	void GDTSetEntry(uint64_t *gdt, int16_t index, uint32_t base, uint32_t limit, int32_t flags)
	{
		gdt[index] = limit & INT64_C(0xffff);
		gdt[index] |= (base & INT64_C(0xffffff)) << 16;
		gdt[index] |= (flags & INT64_C(0xff)) << 40;
		gdt[index] |= ((limit >> 16) & INT64_C(0xf)) << 48;
		gdt[index] |= ((flags >> 8) & INT64_C(0xff)) << 52;
		gdt[index] |= ((base >> 24) & INT64_C(0xff)) << 56;
	}

	void GDTSetTSSEntry(uint64_t *gdt, int16_t index, Sys::TSS *tss)
	{
		uint32_t base  = reinterpret_cast<uint32_t>(tss);
		uint32_t limit = sizeof(Sys::TSS);
		int32_t  flags = GDT_FLAG_TSS | GDT_FLAG_PRESENT | GDT_FLAG_RING3;

		gdt[index] = limit & INT64_C(0xffff);
		gdt[index] |= (base & INT64_C(0xffffff)) << 16;
		gdt[index] |= (flags & INT64_C(0xff)) << 40;
		gdt[index] |= (((limit >> 16) & INT64_C(0xf)) | 0x40) << 48;
		gdt[index] |= ((flags >> 8) & INT64_C(0xff)) << 52;
		gdt[index] |= ((base >> 24) & INT64_C(0xff)) << 56;

		return;
	}


	void GDTInit(uint64_t *gdt, Sys::TSS *tss, void *cpuData)
	{
		struct 
		{
			uint16_t limit;
			void *pointer;
		} __attribute__((packed)) gdtp;

		gdtp.limit = (GDT_ENTRIES * sizeof(uint64_t)) - 1;
		gdtp.pointer = gdt;


		GDTSetEntry(gdt, 0, 0x0, 0x0, 0);
		GDTSetEntry(gdt, 1, 0x0, 0xffffffff, GDT_FLAG_SEGMENT | GDT_FLAG_32_BIT | GDT_FLAG_CODESEG | GDT_FLAG_4K | GDT_FLAG_PRESENT);
		GDTSetEntry(gdt, 2, 0x0, 0xffffffff, GDT_FLAG_SEGMENT | GDT_FLAG_32_BIT | GDT_FLAG_DATASEG | GDT_FLAG_4K | GDT_FLAG_PRESENT);
		GDTSetEntry(gdt, 3, 0x0, 0xffffffff, GDT_FLAG_SEGMENT | GDT_FLAG_32_BIT | GDT_FLAG_CODESEG | GDT_FLAG_4K | GDT_FLAG_PRESENT | GDT_FLAG_RING3);
		GDTSetEntry(gdt, 4, 0x0, 0xffffffff, GDT_FLAG_SEGMENT | GDT_FLAG_32_BIT | GDT_FLAG_DATASEG | GDT_FLAG_4K | GDT_FLAG_PRESENT | GDT_FLAG_RING3);
		GDTSetEntry(gdt, 5, reinterpret_cast<uint32_t>(cpuData), 0xffffffff, GDT_FLAG_SEGMENT | GDT_FLAG_32_BIT | GDT_FLAG_DATASEG | GDT_FLAG_4K | GDT_FLAG_PRESENT | GDT_FLAG_RING3);
		GDTSetTSSEntry(gdt, 6, tss);

		__asm__ volatile("lgdtl %0\r\n"
		                 "ljmpl %1, $_fakeLabel \n\t _fakeLabel: \n\t" : : "m" (gdtp), "i" (0x8));

		__asm__ volatile("mov $0x10, %ax;\r\n"
		                 "mov %ax, %ds;\r\n"
		                 "mov %ax, %es;\r\n"
		                 "mov %ax, %ss\r\n"
		                 "mov $0x28, %eax;\r\n"
		                 "mov %ax, %fs\r\n"
		                 "mov $0x0, %eax;\r\n"
		                 "mov %ax, %gs");

		__asm__ volatile("ltr %%ax" : : "a" (0x30));
	}
}
