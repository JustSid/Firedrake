//
//  trampoline.h
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

#ifndef _TRAMPOLINE_H_
#define _TRAMPOLINE_H_

#include <prefix.h>
#include <libc/stdint.h>
#include <machine/cpu.h>
#include <machine/memory/memory.h>
#include <machine/gdt.h>
#include "idt.h"

namespace ir
{
	typedef struct
	{
		uint8_t buffer[VM_PAGE_SIZE];

		uint32_t *page_directory;

		uint64_t gdt[GDT_ENTRIES];
		uint64_t idt[IDT_ENTRIES];

		tss_t tss;
	} trampoline_map_t;

	trampoline_map_t *get_trampoline_map();
	kern_return_t trampoline_init();
}

#endif /* _TRAMPOLINE_H_ */
