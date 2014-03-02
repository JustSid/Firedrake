//
//  heap.cpp
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
#include "heap.h"
#include "memory.h"

#define kHeapAllocationExtraPadding 16

namespace mm
{
	// --------------------
	// MARK: -
	// MARK: zone_allocation
	// --------------------

	typedef struct
	{
		heap_zone_allocation_type type;
		uint8_t size;
		uint16_t offset;
	} zone_allocation_tiny_t;

	typedef struct
	{
		heap_zone_allocation_type type;
		size_t size;
		uintptr_t pointer;
	} zone_allocation_t;

	// --------------------
	// MARK: -
	// MARK: allocation_proxy
	// --------------------

	allocation_proxy::allocation_proxy(void *allocation, const heap_zone *zone) :
		_allocation(allocation),
		_zone(const_cast<heap_zone *>(zone))
	{
		_tiny = _zone->is_tiny();
	}

	heap_zone_allocation_type allocation_proxy::get_type() const
	{
		// The layout of the type is the same for all allocations
		zone_allocation_t *allocation = reinterpret_cast<zone_allocation_t *>(_allocation);
		return allocation->type;
	}

	size_t allocation_proxy::get_size() const
	{
		if(_tiny)
		{
			zone_allocation_tiny_t *allocation = reinterpret_cast<zone_allocation_tiny_t *>(_allocation);
			return allocation->size;
		}
		else
		{
			zone_allocation_t *allocation = reinterpret_cast<zone_allocation_t *>(_allocation);
			return allocation->size;
		}
	}

	void *allocation_proxy::get_pointer() const
	{
		if(_tiny)
		{
			zone_allocation_tiny_t *allocation = reinterpret_cast<zone_allocation_tiny_t *>(_allocation);
			return reinterpret_cast<void *>(_zone->_begin + allocation->offset);
		}
		else
		{
			zone_allocation_t *allocation = reinterpret_cast<zone_allocation_t *>(_allocation);
			return reinterpret_cast<void *>(allocation->pointer);
		}
	}

	kern_return_t allocation_proxy::free()
	{
		if(get_type() == heap_zone_allocation_type::used)
			return _zone->free_allocation(_allocation);

		return KERN_INVALID_ARGUMENT;
	}

	kern_return_t allocation_proxy::use_with_size(void *&outPointer, size_t size)
	{
		if(get_type() != heap_zone_allocation_type::free)
			return KERN_INVALID_ARGUMENT;

		kern_return_t result = _zone->use_allocation(outPointer, _allocation, size);
		return result;
	}


	// --------------------
	// MARK: -
	// MARK: heap_zone
	// --------------------

	static inline heap_zone_type _heap_zoneype_for_size(size_t size)
	{
		if(size > 2048)
			return heap_zone_type::large;

		if(size > 256)
			return heap_zone_type::medium;

		if(size > 64)
			return heap_zone_type::small;

		return heap_zone_type::tiny;
	}

	heap_zone::heap_zone(uint8_t *start, uint8_t *end, size_t pages, heap_zone_type type) :
		_type(type),
		_pages(pages),
		_begin(start),
		_end(end)
	{
		size_t allocationSize = (type == heap_zone_type::tiny) ? sizeof(zone_allocation_tiny_t) : sizeof(zone_allocation_t);

		_changes = 0;

		_begin = start;
		_end   = end;

		_freeSize = ((pages - 1) * VM_PAGE_SIZE); // Account for the page occupied by the zone itself

		_maxAllocations  = (VM_PAGE_SIZE - sizeof(heap_zone)) / allocationSize;
		_allocations     = 0;
		_freeAllocations = 0;

		_firstAllocation = (reinterpret_cast<uint8_t *>(this)) + sizeof(heap_zone);
		_lastAllocation  = (reinterpret_cast<uint8_t *>(this)) + (_maxAllocations * allocationSize);

		if(is_tiny())
		{
			size_t left = _freeSize;
			uint16_t offset = 0;

			zone_allocation_tiny_t *allocation = reinterpret_cast<zone_allocation_tiny_t *>(_firstAllocation);
			zone_allocation_tiny_t *last       = reinterpret_cast<zone_allocation_tiny_t *>(_lastAllocation);

			for(; allocation != last; allocation ++)
			{
				if(left > 0)
				{
					allocation->size   = std::min(left, UINT8_MAX);
					allocation->offset = offset;
					allocation->type   = heap_zone_allocation_type::free;

					offset += allocation->size;
					left   -= allocation->size;

					_allocations ++;
					_freeAllocations ++;
				}
				else
				{
					allocation->type   = heap_zone_allocation_type::unused;
					allocation->offset = UINT16_MAX;
				}
			}
		}
		else
		{
			_allocations = 1;
			_freeAllocations = 1;

			zone_allocation_t *allocation = reinterpret_cast<zone_allocation_t *>(_firstAllocation);
			zone_allocation_t *last       = reinterpret_cast<zone_allocation_t *>(_lastAllocation);

			allocation->type    = heap_zone_allocation_type::free;
			allocation->size    = _freeSize;
			allocation->pointer = reinterpret_cast<uintptr_t>(_begin);

			for(allocation ++; allocation != last; allocation ++)
			{
				allocation->pointer = 0x0;
				allocation->type    = heap_zone_allocation_type::unused;
			}
		}
	}

	void heap_zone::operator delete(void *ptr)
	{
		heap_zone *zone = static_cast<heap_zone *>(ptr);
		mm::free(zone, vm::get_kernel_directory(), zone->_pages);
	}


	bool heap_zone::is_tiny() const
	{
		return (_type == heap_zone_type::tiny);
	}

	bool heap_zone::can_allocate(size_t size) const
	{
		return (_freeSize >= size && _allocations < _maxAllocations);
	}

	bool heap_zone::contains_allocation(void *ptr) const
	{
		uint8_t *pointer = reinterpret_cast<uint8_t *>(ptr);
		return (pointer >= _begin && pointer < _end);
	}

	void heap_zone::defragment()
	{
		size_t threshold = is_tiny() ? 100 : 20;

		if(_changes >= threshold && _freeAllocations > 2)
		{
			if(is_tiny())
			{
				zone_allocation_tiny_t *allocation = reinterpret_cast<zone_allocation_tiny_t *>(_firstAllocation);
				zone_allocation_tiny_t *last       = reinterpret_cast<zone_allocation_tiny_t *>(_lastAllocation);

				for(; allocation != last; allocation ++)
				{
					while(allocation->type == heap_zone_allocation_type::free)
					{
						zone_allocation_tiny_t *next = static_cast<zone_allocation_tiny_t *>(find_allocation_for_pointer(_begin + allocation->offset + allocation->size));
						if(!next || next->type != heap_zone_allocation_type::free)
							break;

						size_t size = allocation->size;
						if(size + next->size > UINT8_MAX)
							break;

						allocation->size += next->size;

						next->type   = heap_zone_allocation_type::unused;
						next->offset = UINT16_MAX;

						_allocations --;
						_freeAllocations --;
					}
				}
			}
			else
			{
				zone_allocation_t *allocation = reinterpret_cast<zone_allocation_t *>(_firstAllocation);
				zone_allocation_t *last       = reinterpret_cast<zone_allocation_t *>(_lastAllocation);

				for(; allocation != last; allocation ++)
				{
					while(allocation->type == heap_zone_allocation_type::free)
					{
						zone_allocation_t *next = static_cast<zone_allocation_t *>(find_allocation_for_pointer(reinterpret_cast<void *>(allocation->pointer + allocation->size)));
						if(!next || next->type != heap_zone_allocation_type::free)
							break;

						allocation->size += next->size;

						next->type    = heap_zone_allocation_type::unused;
						next->pointer = 0;

						_allocations --;
						_freeAllocations --;
					}
				}
			}

			_changes = 0;
		}
	}

	bool heap_zone::free_allocation_for_size(allocation_proxy &proxy, size_t size) const
	{
		if(is_tiny())
		{
			zone_allocation_tiny_t *allocation = reinterpret_cast<zone_allocation_tiny_t *>(_firstAllocation);
			zone_allocation_tiny_t *last       = reinterpret_cast<zone_allocation_tiny_t *>(_lastAllocation);

			for(; allocation != last; allocation ++)
			{
				if(allocation->type == heap_zone_allocation_type::free && allocation->size >= size)
				{
					proxy = allocation_proxy(allocation, this);
					return true;
				}
			}
		}
		else
		{
			zone_allocation_t *allocation = reinterpret_cast<zone_allocation_t *>(_firstAllocation);
			zone_allocation_t *last       = reinterpret_cast<zone_allocation_t *>(_lastAllocation);

			for(; allocation != last; allocation ++)
			{
				if(allocation->type == heap_zone_allocation_type::free && allocation->size >= size)
				{
					proxy = allocation_proxy(allocation, this);
					return true;
				}
			}
		}

		return false;
	}

	bool heap_zone::allocation_for_pointer(allocation_proxy &proxy, void *ptr) const
	{
		void *allocation = find_allocation_for_pointer(ptr);
		if(allocation)
		{
			proxy = allocation_proxy(allocation, this);
			return true;
		}

		return false;
	}

	bool heap_zone::unused_allocation(allocation_proxy &proxy) const
	{
		void *allocation = find_unused_allocation();
		if(allocation)
		{
			proxy = allocation_proxy(allocation, this);
			return true;
		}

		return false;
	}

	void *heap_zone::find_unused_allocation() const
	{
		if(is_tiny())
		{
			zone_allocation_tiny_t *allocation = reinterpret_cast<zone_allocation_tiny_t *>(_firstAllocation);
			zone_allocation_tiny_t *last       = reinterpret_cast<zone_allocation_tiny_t *>(_lastAllocation);

			for(; allocation != last; allocation ++)
			{
				if(allocation->type == heap_zone_allocation_type::unused)
					return allocation;
			}
		}
		else
		{
			zone_allocation_t *allocation = reinterpret_cast<zone_allocation_t *>(_firstAllocation);
			zone_allocation_t *last       = reinterpret_cast<zone_allocation_t *>(_lastAllocation);

			for(; allocation != last; allocation ++)
			{
				if(allocation->type == heap_zone_allocation_type::unused)
					return allocation;
			}
		}

		return nullptr;
	}

	void *heap_zone::find_allocation_for_pointer(void *ptr) const
	{
		if(is_tiny())
		{
			zone_allocation_tiny_t *allocation = reinterpret_cast<zone_allocation_tiny_t *>(_firstAllocation);
			zone_allocation_tiny_t *last       = reinterpret_cast<zone_allocation_tiny_t *>(_lastAllocation);

			uint8_t	*pointer = reinterpret_cast<uint8_t *>(ptr);

			for(; allocation != last; allocation ++)
			{
				if(pointer == _begin + allocation->offset && allocation->type == heap_zone_allocation_type::used)
					return allocation;
			}
		}
		else
		{
			zone_allocation_t *allocation = reinterpret_cast<zone_allocation_t *>(_firstAllocation);
			zone_allocation_t *last       = reinterpret_cast<zone_allocation_t *>(_lastAllocation);

			uintptr_t pointer = reinterpret_cast<uintptr_t>(ptr);

			for(; allocation != last; allocation ++)
			{
				if(pointer == allocation->pointer && allocation->type == heap_zone_allocation_type::used)
					return allocation;
			}
		}

		return nullptr;
	}

	kern_return_t heap_zone::free_allocation(void *tallocation)
	{
		if(is_tiny())
		{
			zone_allocation_tiny_t *allocation = reinterpret_cast<zone_allocation_tiny_t *>(tallocation);

			allocation->type = heap_zone_allocation_type::free;
			_freeSize += allocation->size;
		}
		else
		{
			zone_allocation_t *allocation = reinterpret_cast<zone_allocation_t *>(tallocation);

			allocation->type = heap_zone_allocation_type::free;
			_freeSize += allocation->size;
		}

		_freeAllocations ++;
		_changes ++;

		return KERN_INVALID_ARGUMENT;
	}

	kern_return_t heap_zone::use_allocation(void *&outPointer, void *tallocation, size_t size)
	{
		size_t required = size + kHeapAllocationExtraPadding;

		if(is_tiny())
		{
			zone_allocation_tiny_t *allocation = reinterpret_cast<zone_allocation_tiny_t *>(tallocation);
			if(allocation->size >= size)
			{
				allocation->type = heap_zone_allocation_type::used;

				if(allocation->size > required)
				{
					zone_allocation_tiny_t *unused = reinterpret_cast<zone_allocation_tiny_t *>(find_unused_allocation());
					if(unused)
					{
						unused->size   = allocation->size - required;
						unused->offset = allocation->offset + required;
						unused->type   = heap_zone_allocation_type::free;

						allocation->size = required;

						_freeAllocations ++;
						_allocations ++;
					}
				}

				_freeAllocations --;
				_freeSize -= allocation->size;

				outPointer = reinterpret_cast<void *>(_begin + allocation->offset);

				return KERN_SUCCESS;
			}
		}
		else
		{
			zone_allocation_t *allocation = reinterpret_cast<zone_allocation_t *>(tallocation);
			if(allocation->size >= size)
			{
				allocation->type = heap_zone_allocation_type::used;

				if(allocation->size > required)
				{
					zone_allocation_t *unused = reinterpret_cast<zone_allocation_t *>(find_unused_allocation());
					if(unused)
					{
						unused->size    = allocation->size - required;
						unused->pointer = allocation->pointer + required;
						unused->type    = heap_zone_allocation_type::free;

						allocation->size = required;

						_freeAllocations ++;
						_allocations ++;
					}
				}

				_freeAllocations --;
				_freeSize -= allocation->size;

				outPointer = reinterpret_cast<void *>(allocation->pointer);

				return KERN_SUCCESS;
			}
		}

		return KERN_INVALID_ARGUMENT;
	}

	kern_return_t heap_zone::create(heap_zone *&zone, size_t size)
	{
		heap_zone_type type = _heap_zoneype_for_size(size);
		size_t pages;

		switch(type)
		{
			case heap_zone_type::tiny:
				pages = 1;
				break;

			case heap_zone_type::small:
				pages = 5;
				break;

			case heap_zone_type::medium:
				pages = 20;
				break;

			case heap_zone_type::large:
				pages = VM_PAGE_COUNT(size);
				break;
		}

		pages ++;
		uint8_t *buffer = mm::alloc<uint8_t>(vm::get_kernel_directory(), pages, VM_FLAGS_KERNEL);

		if(!buffer)
			return KERN_NO_MEMORY;

		zone = new(buffer) heap_zone(buffer + VM_PAGE_SIZE, buffer + (pages * VM_PAGE_SIZE), pages, type);
		return KERN_SUCCESS;
	}

	// --------------------
	// MARK: -
	// MARK: heap
	// --------------------

	heap::heap(uint32_t flags) :
		_lock(SPINLOCK_INIT),
		_flags(flags)
	{}

	heap::~heap()
	{
		heap_zone *zone = _zones.get_head();
		while(zone)
		{
			heap_zone *temp = zone;
			zone = zone->get_next();

			delete temp;
		}
	}

	void *heap::operator new(__unused size_t size)
	{
		void *buffer = mm::alloc<void>(vm::get_kernel_directory(), 1, VM_FLAGS_KERNEL);
		return buffer;
	}

	void heap::operator delete(void *ptr)
	{
		mm::free(ptr, vm::get_kernel_directory(), 1);
	}



	void *heap::allocate_from_zone(heap_zone *zone, size_t size)
	{
		allocation_proxy proxy;

		void *pointer;
		bool result = zone->free_allocation_for_size(proxy, size);

		if(result)
		{
			kern_return_t retVal = proxy.use_with_size(pointer, size);

			if(retVal == KERN_SUCCESS)
				return pointer;
		}

		return nullptr;
	}

	void *heap::allocate(size_t size)
	{
		if(_flags & heap::flag_aligned)
			size += (size % 4);

		void *pointer = nullptr;
		heap_zone_type type = _heap_zoneype_for_size(size);

		spinlock_lock(&_lock);

		// Large allocations require a complete zone for themselves
		if(type == heap_zone_type::large)
		{
			heap_zone *zone;
			kern_return_t result = heap_zone::create(zone, size);

			if(result == KERN_SUCCESS)
			{
				pointer = allocate_from_zone(zone, size);
				_zones.push_back(zone);
			}

			spinlock_unlock(&_lock);
			return pointer;
		}

		// Try to find a zone which can hold the allocation
		heap_zone *zone = _zones.get_head();
		while(zone)
		{
			if(zone->get_type() == type && zone->can_allocate(size))
			{
				pointer = allocate_from_zone(zone, size);
				if(pointer)
					break;
			}

			zone = zone->get_next();
		}

		// Create a new zone if needed
		if(!pointer)
		{
			heap_zone *zone;
			kern_return_t result = heap_zone::create(zone, size);

			if(result == KERN_SUCCESS)
			{
				pointer = allocate_from_zone(zone, size);
				_zones.push_back(zone);
			}
		}

		spinlock_unlock(&_lock);

		if(pointer && _flags & heap::flag_secure)
			memset(pointer, 0, size);

		return pointer;
	}

	void heap::free(void *ptr)
	{
		spinlock_lock(&_lock);

		bool didFree = false;
		allocation_proxy proxy;

		heap_zone *zone = _zones.get_head();
		while(zone)
		{
			bool holdsAllocation = zone->contains_allocation(ptr);

			if(holdsAllocation && zone->allocation_for_pointer(proxy, ptr))
			{
				if(_flags & heap::flag_secure)
					memset(proxy.get_pointer(), 0, proxy.get_size());

				didFree = true;
				proxy.free();

				if(zone->is_empty())
				{
					delete zone;
				}
				else
				{
					zone->defragment();
				}
			}

			if(holdsAllocation)
				break;

			zone = zone->get_next();
		}

		spinlock_unlock(&_lock);

		if(!didFree)
			kprintf("Tried to free unknown pointer: %p\n", ptr);
	}

	// --------------------
	// MARK: -
	// MARK: Accessor/Initialization
	// --------------------

	static heap *_generic_heap = nullptr;

	heap *get_generic_heap()
	{
		return _generic_heap;
	}

	kern_return_t heap_init()
	{
		_generic_heap = new heap(heap::flag_aligned);
		return (_generic_heap != nullptr) ? KERN_SUCCESS : KERN_NO_MEMORY;
	}
}
