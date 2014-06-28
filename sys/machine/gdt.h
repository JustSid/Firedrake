//
//  gdt.h
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

#ifndef _GDT_H_
#define _GDT_H_

#include <prefix.h>
#include <libc/stdint.h>
#include "cpu.h"

#define GDT_FLAG_DATASEG  0x02
#define GDT_FLAG_CODESEG  0x0a
#define GDT_FLAG_TSS      0x09

#define GDT_FLAG_SEGMENT  0x10
#define GDT_FLAG_RING0    0x00
#define GDT_FLAG_RING3    0x60
#define GDT_FLAG_PRESENT  0x80

#define GDT_FLAG_4K       0x800
#define GDT_FLAG_32_BIT   0x400

#define GDT_ENTRIES 6

namespace Sys
{
	void GDTInit(uint64_t *gdt, Sys::TSS *tss);
}

#endif /* _GDT_H_ */
