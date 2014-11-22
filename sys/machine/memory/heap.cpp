//
//  Heap.cpp
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

#include <libcpp/new.h>
#include <libcpp/algorithm.h>
#include <libc/string.h>
#include <kern/kprintf.h>
#include <kern/panic.h>
#include "Heap.h"
#include "memory.h"

namespace Sys
{
	constexpr size_t kHeapAllocationExtraPadding = 16;

	// --------------------
	// MARK: -
	// MARK: zone_allocation
	// --------------------

	typedef struct
	{
		HeapZoneAllocationType type;
		uint8_t size;
		uint16_t offset;
	} ZoneAllocationTiny;

	typedef struct
	{
		HeapZoneAllocationType type;
		size_t size;
		uintptr_t pointer;
	} ZoneAllocation;

	// --------------------
	// MARK: -
	// MARK: AllocationProxy
	// --------------------

	AllocationProxy::AllocationProxy(void *allocation, const HeapZone *zone) :
		_allocation(allocation),
		_zone(const_cast<HeapZone *>(zone))
	{
		_tiny = _zone->IsTiny();
	}

	HeapZoneAllocationType AllocationProxy::GetType() const
	{
		// The layout of the type is the same for all allocations
		ZoneAllocation *allocation = reinterpret_cast<ZoneAllocation *>(_allocation);
		return allocation->type;
	}

	size_t AllocationProxy::GetSize() const
	{
		if(_tiny)
		{
			ZoneAllocationTiny *allocation = reinterpret_cast<ZoneAllocationTiny *>(_allocation);
			return allocation->size;
		}
		else
		{
			ZoneAllocation *allocation = reinterpret_cast<ZoneAllocation *>(_allocation);
			return allocation->size;
		}
	}

	void *AllocationProxy::GetPointer() const
	{
		if(_tiny)
		{
			ZoneAllocationTiny *allocation = reinterpret_cast<ZoneAllocationTiny *>(_allocation);
			return reinterpret_cast<void *>(_zone->_begin + allocation->offset);
		}
		else
		{
			ZoneAllocation *allocation = reinterpret_cast<ZoneAllocation *>(_allocation);
			return reinterpret_cast<void *>(allocation->pointer);
		}
	}

	KernReturn<void> AllocationProxy::Free()
	{
		if(GetType() == HeapZoneAllocationType::Used)
			return _zone->FreeAllocation(_allocation);

		return Error(KERN_INVALID_ARGUMENT);
	}

	KernReturn<void *> AllocationProxy::UseWithSize( size_t size)
	{
		if(GetType() != HeapZoneAllocationType::Free)
			return Error(KERN_INVALID_ARGUMENT);

		return _zone->UseAllocation(_allocation, size);
	}


	// --------------------
	// MARK: -
	// MARK: HeapZone
	// --------------------

	static inline HeapZoneType _HeapZoneTypeForSize(size_t size)
	{
		if(size > 2048)
			return HeapZoneType::Large;

		if(size > 256)
			return HeapZoneType::Medium;

		if(size > 64)
			return HeapZoneType::Small;

		return HeapZoneType::Tiny;
	}

	HeapZone::HeapZone(uint8_t *start, uint8_t *end, size_t pages, HeapZoneType type) :
		_type(type),
		_pages(pages),
		_begin(start),
		_end(end)
	{
		size_t allocationSize = (type == HeapZoneType::Tiny) ? sizeof(ZoneAllocationTiny) : sizeof(ZoneAllocation);

		_changes = 0;

		_begin = start;
		_end   = end;

		_freeSize = ((pages - 1) * VM_PAGE_SIZE); // Account for the page occupied by the zone itself

		_maxAllocations  = (VM_PAGE_SIZE - sizeof(HeapZone)) / allocationSize;
		_allocations     = 0;
		_freeAllocations = 0;

		_firstAllocation = (reinterpret_cast<uint8_t *>(this)) + sizeof(HeapZone);
		_lastAllocation  = (reinterpret_cast<uint8_t *>(this)) + (_maxAllocations * allocationSize);

		if(IsTiny())
		{
			size_t left = _freeSize;
			uint16_t offset = 0;

			ZoneAllocationTiny *allocation = reinterpret_cast<ZoneAllocationTiny *>(_firstAllocation);
			ZoneAllocationTiny *last       = reinterpret_cast<ZoneAllocationTiny *>(_lastAllocation);

			for(; allocation != last; allocation ++)
			{
				if(left > 0)
				{
					allocation->size   = std::min(left, UINT8_MAX);
					allocation->offset = offset;
					allocation->type   = HeapZoneAllocationType::Free;

					offset += allocation->size;
					left   -= allocation->size;

					_allocations ++;
					_freeAllocations ++;
				}
				else
				{
					allocation->type   = HeapZoneAllocationType::Unused;
					allocation->offset = UINT16_MAX;
				}
			}
		}
		else
		{
			_allocations = 1;
			_freeAllocations = 1;

			ZoneAllocation *allocation = reinterpret_cast<ZoneAllocation *>(_firstAllocation);
			ZoneAllocation *last       = reinterpret_cast<ZoneAllocation *>(_lastAllocation);

			allocation->type    = HeapZoneAllocationType::Free;
			allocation->size    = _freeSize;
			allocation->pointer = reinterpret_cast<uintptr_t>(_begin);

			for(allocation ++; allocation != last; allocation ++)
			{
				allocation->pointer = 0x0;
				allocation->type    = HeapZoneAllocationType::Unused;
			}
		}
	}

	void HeapZone::operator delete(void *ptr)
	{
		HeapZone *zone = static_cast<HeapZone *>(ptr);
		Free(zone, VM::Directory::GetKernelDirectory(), zone->_pages);
	}


	bool HeapZone::IsTiny() const
	{
		return (_type == HeapZoneType::Tiny);
	}

	bool HeapZone::CanAllocate(size_t size) const
	{
		return (_freeSize >= size && _allocations < _maxAllocations);
	}

	bool HeapZone::ContainsAllocation(void *ptr) const
	{
		uint8_t *pointer = reinterpret_cast<uint8_t *>(ptr);
		return (pointer >= _begin && pointer < _end);
	}

	void HeapZone::Defragment()
	{
		size_t threshold = IsTiny() ? 100 : 20;

		if(_changes >= threshold && _freeAllocations > 2)
		{
			if(IsTiny())
			{
				ZoneAllocationTiny *allocation = reinterpret_cast<ZoneAllocationTiny *>(_firstAllocation);
				ZoneAllocationTiny *last       = reinterpret_cast<ZoneAllocationTiny *>(_lastAllocation);

				for(; allocation != last; allocation ++)
				{
					while(allocation->type == HeapZoneAllocationType::Free)
					{
						ZoneAllocationTiny *next = static_cast<ZoneAllocationTiny *>(FindAllocationForPointer(_begin + allocation->offset + allocation->size));
						if(!next || next->type != HeapZoneAllocationType::Free)
							break;

						size_t size = allocation->size;
						if(size + next->size > UINT8_MAX)
							break;

						allocation->size += next->size;

						next->type   = HeapZoneAllocationType::Unused;
						next->offset = UINT16_MAX;

						_allocations --;
						_freeAllocations --;
					}
				}
			}
			else
			{
				ZoneAllocation *allocation = reinterpret_cast<ZoneAllocation *>(_firstAllocation);
				ZoneAllocation *last       = reinterpret_cast<ZoneAllocation *>(_lastAllocation);

				for(; allocation != last; allocation ++)
				{
					while(allocation->type == HeapZoneAllocationType::Free)
					{
						ZoneAllocation *next = static_cast<ZoneAllocation *>(FindAllocationForPointer(reinterpret_cast<void *>(allocation->pointer + allocation->size)));
						if(!next || next->type != HeapZoneAllocationType::Free)
							break;

						allocation->size += next->size;

						next->type    = HeapZoneAllocationType::Unused;
						next->pointer = 0;

						_allocations --;
						_freeAllocations --;
					}
				}
			}

			_changes = 0;
		}
	}

	bool HeapZone::FreeAllocationForSize(AllocationProxy &proxy, size_t size) const
	{
		if(IsTiny())
		{
			ZoneAllocationTiny *allocation = reinterpret_cast<ZoneAllocationTiny *>(_firstAllocation);
			ZoneAllocationTiny *last       = reinterpret_cast<ZoneAllocationTiny *>(_lastAllocation);

			for(; allocation != last; allocation ++)
			{
				if(allocation->type == HeapZoneAllocationType::Free && allocation->size >= size)
				{
					proxy = AllocationProxy(allocation, this);
					return true;
				}
			}
		}
		else
		{
			ZoneAllocation *allocation = reinterpret_cast<ZoneAllocation *>(_firstAllocation);
			ZoneAllocation *last       = reinterpret_cast<ZoneAllocation *>(_lastAllocation);

			for(; allocation != last; allocation ++)
			{
				if(allocation->type == HeapZoneAllocationType::Free && allocation->size >= size)
				{
					proxy = AllocationProxy(allocation, this);
					return true;
				}
			}
		}

		return false;
	}

	bool HeapZone::AllocationForPointer(AllocationProxy &proxy, void *ptr) const
	{
		void *allocation = FindAllocationForPointer(ptr);
		if(allocation)
		{
			proxy = AllocationProxy(allocation, this);
			return true;
		}

		return false;
	}

	bool HeapZone::UnusedAllocation(AllocationProxy &proxy) const
	{
		void *allocation = FindUnusedAllocation();
		if(allocation)
		{
			proxy = AllocationProxy(allocation, this);
			return true;
		}

		return false;
	}

	void *HeapZone::FindUnusedAllocation() const
	{
		if(IsTiny())
		{
			ZoneAllocationTiny *allocation = reinterpret_cast<ZoneAllocationTiny *>(_firstAllocation);
			ZoneAllocationTiny *last       = reinterpret_cast<ZoneAllocationTiny *>(_lastAllocation);

			for(; allocation != last; allocation ++)
			{
				if(allocation->type == HeapZoneAllocationType::Unused)
					return allocation;
			}
		}
		else
		{
			ZoneAllocation *allocation = reinterpret_cast<ZoneAllocation *>(_firstAllocation);
			ZoneAllocation *last       = reinterpret_cast<ZoneAllocation *>(_lastAllocation);

			for(; allocation != last; allocation ++)
			{
				if(allocation->type == HeapZoneAllocationType::Unused)
					return allocation;
			}
		}

		return nullptr;
	}

	void *HeapZone::FindAllocationForPointer(void *ptr) const
	{
		if(IsTiny())
		{
			ZoneAllocationTiny *allocation = reinterpret_cast<ZoneAllocationTiny *>(_firstAllocation);
			ZoneAllocationTiny *last       = reinterpret_cast<ZoneAllocationTiny *>(_lastAllocation);

			uint8_t	*pointer = reinterpret_cast<uint8_t *>(ptr);

			for(; allocation != last; allocation ++)
			{
				if(pointer == _begin + allocation->offset && allocation->type == HeapZoneAllocationType::Used)
					return allocation;
			}
		}
		else
		{
			ZoneAllocation *allocation = reinterpret_cast<ZoneAllocation *>(_firstAllocation);
			ZoneAllocation *last       = reinterpret_cast<ZoneAllocation *>(_lastAllocation);

			uintptr_t pointer = reinterpret_cast<uintptr_t>(ptr);

			for(; allocation != last; allocation ++)
			{
				if(pointer == allocation->pointer && allocation->type == HeapZoneAllocationType::Used)
					return allocation;
			}
		}

		return nullptr;
	}

	KernReturn<void> HeapZone::FreeAllocation(void *tallocation)
	{
		if(IsTiny())
		{
			ZoneAllocationTiny *allocation = reinterpret_cast<ZoneAllocationTiny *>(tallocation);

			allocation->type = HeapZoneAllocationType::Free;
			_freeSize += allocation->size;
		}
		else
		{
			ZoneAllocation *allocation = reinterpret_cast<ZoneAllocation *>(tallocation);

			allocation->type = HeapZoneAllocationType::Free;
			_freeSize += allocation->size;
		}

		_freeAllocations ++;
		_changes ++;

		return Error(KERN_SUCCESS);
	}

	KernReturn<void *> HeapZone::UseAllocation(void *tallocation, size_t size)
	{
		size_t required = size + kHeapAllocationExtraPadding;

		if(IsTiny())
		{
			ZoneAllocationTiny *allocation = reinterpret_cast<ZoneAllocationTiny *>(tallocation);
			if(allocation->size >= size)
			{
				allocation->type = HeapZoneAllocationType::Used;

				if(allocation->size > required)
				{
					ZoneAllocationTiny *unused = reinterpret_cast<ZoneAllocationTiny *>(FindUnusedAllocation());
					if(unused)
					{
						unused->size   = allocation->size - required;
						unused->offset = allocation->offset + required;
						unused->type   = HeapZoneAllocationType::Free;

						allocation->size = required;

						_freeAllocations ++;
						_allocations ++;
					}
				}

				_freeAllocations --;
				_freeSize -= allocation->size;

				return reinterpret_cast<void *>(_begin + allocation->offset);
			}
		}
		else
		{
			ZoneAllocation *allocation = reinterpret_cast<ZoneAllocation *>(tallocation);
			if(allocation->size >= size)
			{
				allocation->type = HeapZoneAllocationType::Used;

				if(allocation->size > required)
				{
					ZoneAllocation *unused = reinterpret_cast<ZoneAllocation *>(FindUnusedAllocation());
					if(unused)
					{
						unused->size    = allocation->size - required;
						unused->pointer = allocation->pointer + required;
						unused->type    = HeapZoneAllocationType::Free;

						allocation->size = required;

						_freeAllocations ++;
						_allocations ++;
					}
				}

				_freeAllocations --;
				_freeSize -= allocation->size;

				return reinterpret_cast<void *>(allocation->pointer);
			}
		}

		return Error(KERN_INVALID_ARGUMENT);
	}

	KernReturn<HeapZone *> HeapZone::Create(size_t size)
	{
		HeapZoneType type = _HeapZoneTypeForSize(size);
		size_t pages;

		switch(type)
		{
			case HeapZoneType::Tiny:
				pages = 1;
				break;

			case HeapZoneType::Small:
				pages = 5;
				break;

			case HeapZoneType::Medium:
				pages = 20;
				break;

			case HeapZoneType::Large:
				pages = VM_PAGE_COUNT(size);
				break;
		}

		pages ++;
		uint8_t *buffer = Alloc<uint8_t>(VM::Directory::GetKernelDirectory(), pages, kVMFlagsKernel);

		if(!buffer)
			return Error(KERN_NO_MEMORY);

		return new(buffer) HeapZone(buffer + VM_PAGE_SIZE, buffer + (pages * VM_PAGE_SIZE), pages, type);
	}

	// --------------------
	// MARK: -
	// MARK: Heap
	// --------------------

	Heap::Heap(Flags flags) :
		_lock(SPINLOCK_INIT),
		_flags(flags)
	{}

	Heap::~Heap()
	{
		HeapZone *zone = _zones.get_head();
		while(zone)
		{
			HeapZone *temp = zone;
			zone = zone->get_next();

			delete temp;
		}
	}

	void *Heap::operator new(__unused size_t size)
	{
		void *buffer = Alloc<void>(VM::Directory::GetKernelDirectory(), 1, kVMFlagsKernel);
		return buffer;
	}

	void Heap::operator delete(void *ptr)
	{
		Sys::Free(ptr, VM::Directory::GetKernelDirectory(), 1);
	}



	void *Heap::AllocateFromZone(HeapZone *zone, size_t size)
	{
		AllocationProxy proxy;

		bool result = zone->FreeAllocationForSize(proxy, size);
		if(result)
		{
			KernReturn<void *> pointer = proxy.UseWithSize(size);

			if(pointer.IsValid())
				return pointer;
		}

		return nullptr;
	}

	void *Heap::Allocate(size_t size)
	{
		if(_flags & Flags::Aligned)
			size += (size % 4);

		void *pointer = nullptr;
		HeapZoneType type = _HeapZoneTypeForSize(size);

		spinlock_lock(&_lock);

		// Large allocations require a complete zone for themselves
		if(type == HeapZoneType::Large)
		{
			KernReturn<HeapZone *> zone = HeapZone::Create(size);

			if(zone.IsValid())
			{
				pointer = AllocateFromZone(zone, size);
				_zones.push_back(zone);
			}

			spinlock_unlock(&_lock);

			if(pointer && _flags & Flags::Secure)
				memset(pointer, 0, size);

			return pointer;
		}

		// Try to find a zone which can hold the allocation
		HeapZone *zone = _zones.get_head();
		while(zone)
		{
			if(zone->GetType() == type && zone->CanAllocate(size))
			{
				pointer = AllocateFromZone(zone, size);
				if(pointer)
					break;
			}

			zone = zone->get_next();
		}

		// Create a new zone if needed
		if(!pointer)
		{
			KernReturn<HeapZone *> zone = HeapZone::Create(size);

			if(zone.IsValid())
			{
				pointer = AllocateFromZone(zone, size);
				_zones.push_back(zone);
			}
		}

		spinlock_unlock(&_lock);

		if(pointer && _flags & Flags::Secure)
			memset(pointer, 0, size);

		return pointer;
	}

	void Heap::Free(void *ptr)
	{
		spinlock_lock(&_lock);

		bool didFree = false;
		AllocationProxy proxy;

		HeapZone *zone = _zones.get_head();
		while(zone)
		{
			bool holdsAllocation = zone->ContainsAllocation(ptr);

			if(holdsAllocation && zone->AllocationForPointer(proxy, ptr))
			{
				if(_flags & Heap::Flags::Secure)
					memset(proxy.GetPointer(), 0, proxy.GetSize());

				didFree = true;
				proxy.Free();

				if(zone->IsEmpty())
				{
					delete zone;
				}
				else
				{
					zone->Defragment();
				}
			}

			if(holdsAllocation)
				break;

			zone = zone->get_next();
		}

		spinlock_unlock(&_lock);

		if(!didFree)
			panic("Tried to free unknown pointer: %p\n", ptr);
	}

	// --------------------
	// MARK: -
	// MARK: Accessor/Initialization
	// --------------------

	static Heap *_genericHeap = nullptr;

	Heap *Heap::GetGenericHeap()
	{
		return _genericHeap;
	}

	KernReturn<void> HeapInit()
	{
		_genericHeap = new Heap(Heap::Flags::Aligned);
		return (_genericHeap != nullptr) ? Error(KERN_SUCCESS) : Error(KERN_NO_MEMORY);
	}
}
