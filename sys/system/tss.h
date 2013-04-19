//
//  tss.h
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

#ifndef _TSS_H_
#define _TSS_H_

#include <prefix.h>

struct tss_s 
{
	uint32_t	back_link;	/* segment number of previous task, if nested */
	uint32_t	esp0;		/* initial stack pointer ... */
	uint32_t	ss0;		/* and segment for ring 0 */
	uint32_t	esp1;		/* initial stack pointer ... */
	uint32_t	ss1;		/* and segment for ring 1 */
	uint32_t	esp2;		/* initial stack pointer ... */
	uint32_t	ss2;		/* and segment for ring 2 */
	uint32_t	cr3;		/* CR3 - page table directory physical address */
	uint32_t	eip;
	uint32_t	eflags;
	uint32_t	eax;
	uint32_t	ecx;
	uint32_t	edx;
	uint32_t	ebx;
	uint32_t	esp;		/* current stack pointer */
	uint32_t	ebp;
	uint32_t	esi;
	uint32_t	edi;
	uint32_t	es;
	uint32_t	cs;
	uint32_t	ss;		/* current stack segment */
	uint32_t	ds;
	uint32_t	fs;
	uint32_t	gs;
	uint32_t	ldt;		/* local descriptor table segment */
	uint16_t	trace_trap;	/* trap on switch to this task */
	uint16_t	io_bit_map_offset; /* offset to start of IO permission bit map */
};

#endif /* _TSS_H_ */
