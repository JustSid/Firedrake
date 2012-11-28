//
//  dma.h
//  libkernel
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

#ifndef _LIBKERNEL_DMA_H_
#define _LIBKERNEL_DMA_H_

#include "base.h"
#include "stdint.h"
#include "types.h"

#define kDMARequestFlagContiguous (1 << 0)

#define kDMAMaxPhysicalFragments 32

typedef struct
{
	size_t pfragmentCount;
	uintptr_t pfragments[kDMAMaxPhysicalFragments];
	size_t pfragmentPages[kDMAMaxPhysicalFragments];

	size_t pages;
	vm_address_t vaddress;
} dma_t;

kern_extern dma_t *dma_request(size_t pages, uint32_t flags);
kern_extern void dma_free(dma_t *dma);

#endif /* _LIBKERNEL_DMA_H_ */
