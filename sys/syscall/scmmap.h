//
//  scmmap.h
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

#ifndef _SCMMAP_H_
#define _SCMMAP_H_

#include <types.h>
#include <container/list.h>
#include <memory/memory.h>
#include <scheduler/scheduler.h>

#define PROT_NONE   0x00
#define PROT_READ   0x01
#define PROT_WRITE  0x02
#define PROT_EXEC	0x04

#define MAP_SHARED    0x0001 // Not supported yet
#define MAP_PRIVATE   0x0002
#define MAP_ANONYMOUS 	0x0004 // Must be specified because of lack of file descriptors
#define MAP_FAILED		-1

typedef struct
{
	list_base_t base;

	vm_address_t vaddress;
	uintptr_t paddress;

	size_t length;
	int protection;
} mmap_description_t;

uint32_t mmap_vmflagsForProtectionFlags(int protection);
bool mmap_copyMappings(process_t *target, process_t *source);

#endif
