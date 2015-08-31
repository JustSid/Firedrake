//
//  IOMemoryRegion.cpp
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2015 by Sidney Just
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

#include "IOMemoryRegion.h"

extern "C" void *__libkern_dma_map(uintptr_t physical, size_t pages);
extern "C" void __libkern_dma_free(void *virt, size_t pages);

namespace IO
{
	IODefineMeta(MemoryRegion, Object)

	MemoryRegion *MemoryRegion::InitWithPhysical(uintptr_t physical, size_t pages)
	{
		if(!Object::Init())
			return nullptr;

		_physical = physical;
		_pages = pages;

		_address = __libkern_dma_map(_physical, _pages);

		return this;
	}

	void MemoryRegion::Dealloc()
	{
		if(_address)
			__libkern_dma_free(_address, _pages);

		Object::Dealloc();
	}

	uintptr_t MemoryRegion::GetPhysical() const
	{
		return _physical;
	}
	void *MemoryRegion::GetAddress() const
	{
		return _address;
	}
	size_t MemoryRegion::GetPages() const
	{
		return _pages;
	}
}
