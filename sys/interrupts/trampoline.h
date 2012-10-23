//
//  trampoline.h
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

#ifndef _TRAMPOLINE_H_
#define _TRAMPOLINE_H_

#ifndef __kasm__

#include <types.h>
#include <memory/memory.h>
#include <system/tss.h>
#include <system/gdt.h>
#include "interrupts.h"

#endif /* __kasm__ */

#define IR_TRAMPOLINE_PHYSICAL	0x2000
#define IR_TRAMPOLINE_BEGIN 	0xFFAFF000
#define IR_TRAMPOLINE_PAGES 	2

#define IR_TRAMPOLINE_PAGEDIR (IR_TRAMPOLINE_BEGIN + 0x1000)

#ifndef __kasm__

typedef struct
{
	// Page 1
	uint8_t base[VM_PAGE_SIZE]; // place where the interrupt handlers live

	// Page 2
	vm_page_directory_t pagedir;

	uint64_t gdt[GDT_ENTRIES];
	uint64_t idt[IDT_ENTRIES];
	struct tss_s tss;
} ir_trampoline_map_t;

uintptr_t ir_trampolineResolveFrame(vm_address_t frame);

bool ir_trampoline_init(void *unused);


extern ir_trampoline_map_t *ir_trampoline_map;

#endif /* __kasm__ */
#endif /* _TRAMPOLINE_H_ */
