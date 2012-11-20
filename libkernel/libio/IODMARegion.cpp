//
//  IODMARegion.cpp
//  libio
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

#include "IODMARegion.h"

#ifdef super
#undef super
#endif
#define super IOObject

IORegisterClass(IODMARegion, super)

IODMARegion *IODMARegion::initWithRequest(uint32_t request, size_t pages)
{
	if(!super::init())
		return 0;

	_wrapped = dma_request(pages, request);
	if(!_wrapped)
	{
		release();
		return 0;
	}

	return this;
}

void IODMARegion::free()
{
	if(_wrapped)
		dma_free(_wrapped);

	super::free();
}


size_t IODMARegion::physicalRegionCount() const
{
	return _wrapped->pfragmentCount;
}

uintptr_t IODMARegion::physicalRegion(size_t index) const
{
	return _wrapped->pfragments[index];
}

vm_address_t IODMARegion::virtualRegion() const
{
	return _wrapped->vaddress;
}
