//
//  memory.h
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

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <types.h>
#include "pmemory.h"
#include "vmemory.h"
#include "heap.h"

// Some more high-level allocation system
void *kalloc(size_t bytes); // Kernel
void *ualloc(size_t bytes); // Userland
void kfree(void *ptr);

#define round4kDown(x)	((x) & ~(VM_PAGE_SIZE - 1)) // Rounds down to the previous 4k aligned value
#define round4kUp(x)	round4kDown((x) + VM_PAGE_SIZE - 1) // Rounds up to the next 4k aligned value

#define pageCount(x)	((((x) % VM_PAGE_SIZE) == 0) ? (x) / VM_PAGE_SIZE : ((x) / VM_PAGE_SIZE) + 1)

#endif /* _MEMORY_H_ */
