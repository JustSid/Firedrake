//
//  virtual.h
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

#ifndef _VIRTUAL_H_
#define _VIRTUAL_H_

#include <libc/stdint.h>
#include <kern/kern_return.h>
#include <kern/spinlock.h>

#define VM_PAGE_SHIFT 12
#define VM_DIRECTORY_SHIFT 22

#define VM_PAGE_SIZE (1 << VM_PAGE_SHIFT)
#define VM_PAGE_MASK (~(VM_PAGE_SIZE - 1))

#define VM_PAGE_COUNT(x)      ((((x) + ~VM_PAGE_MASK) & VM_PAGE_MASK) / VM_PAGE_SIZE)
#define VM_PAGE_ALIGN_DOWN(x) ((x) & VM_PAGE_MASK)
#define VM_PAGE_ALIGN_UP(x)   (VM_PAGE_ALIGN_DOWN((x) + ~VM_PAGE_MASK))

#define VM_LOWER_LIMIT 0x1000
#define VM_UPPER_LIMIT 0xfffff000

#endif /* _VIRTUAL_H_ */
