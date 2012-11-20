//
//  dma.c
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

#include "dma.h"
#include "heap.h"

extern bool vm_fulfillDMARequest(dma_t *dma);

dma_t *dma_request(size_t pages, uint32_t flags)
{
	dma_t *dma = halloc(NULL, sizeof(dma_t));
	if(dma)
	{
		dma->vaddress = 0;
		dma->pfragmentCount = 0;

		dma->pages = pages;

		if((flags & kDMARequestFlagContiguous) || pages == 1)
		{
			dma->pfragmentCount = 1;
			dma->pfragmentPages[0] = pages;

			dma->pfragments[0] = pm_alloc(pages);
			if(!dma->pfragments[0])
			{
				hfree(NULL, dma);
				return NULL;
			}
		}
		else
		{
			size_t pagesLeft = pages;
			size_t i = 0;

			while(pagesLeft > 0 && i < kDMAMaxPhysicalFragments)
			{
				size_t npages = pagesLeft;
				uintptr_t pmemory = pm_alloc(npages);

				if(!pmemory)
				{
					if(npages == 1)
					{
						dma_free(dma);
						return NULL;
					}

					npages /= 2;
					continue;
				}

				dma->pfragments[i] = pmemory;
				dma->pfragmentPages[i] = npages;
				
				dma->pfragmentCount ++;
				i ++;
			}
		}

		bool result = vm_fulfillDMARequest(dma);
		if(!result)
		{
			dma_free(dma);
			return NULL;
		}
	}

	return dma;
}

void dma_free(dma_t *dma)
{
	for(size_t i=0; i<dma->pfragmentCount; i++)
	{
		pm_free(dma->pfragments[i], dma->pfragmentPages[i]);
	}

	if(dma->vaddress)
		vm_free(vm_getKernelDirectory(), dma->vaddress, dma->pages);

	hfree(NULL, dma);
}
