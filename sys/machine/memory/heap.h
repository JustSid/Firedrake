//
//  heap.h
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

#ifndef _HEAP_H_
#define _HEAP_H_

#include <prefix.h>
#include <libc/stddef.h>
#include <libc/stdint.h>
#include <libcpp/list.h>
#include <kern/kern_return.h>
#include <kern/spinlock.h>

namespace mm
{
	enum class heap_zone_type
	{
		tiny, // Less than 64 bytes
		small, // Less than 256
		medium, // Less than 2048
		large // Everything else
	};

	enum class heap_zone_allocation_type : uint8_t
	{
		free,
		used,
		unused
	};

	class heap_zone;
	class allocation_proxy
	{
	public:
		friend class heap_zone;

		allocation_proxy() {}

		heap_zone_allocation_type get_type() const;
		size_t get_size() const;
		void *get_pointer() const;

		kern_return_t free();
		kern_return_t use_with_size(void *&outPointer, size_t size);

	private:
		allocation_proxy(void *allocation, const heap_zone *zone);

		heap_zone *_zone;
		void *_allocation;
		bool _tiny;
	};

	class heap_zone : public cpp::list<heap_zone>::node
	{
	public:
		friend class allocation_proxy;

		static kern_return_t create(heap_zone *&zone, size_t size);
		void operator delete(void *ptr);

		bool is_tiny() const;
		bool is_empty() const { return _allocations == _freeAllocations; }
		bool can_allocate(size_t size) const;
		bool contains_allocation(void *ptr) const;

		size_t get_free_size() const { return _freeSize; }
		heap_zone_type get_type() const { return _type; }

		void defragment();

		bool free_allocation_for_size(allocation_proxy &proxy, size_t size) const;
		bool allocation_for_pointer(allocation_proxy &proxy, void *ptr) const;
		bool unused_allocation(allocation_proxy &proxy) const;

	private:
		heap_zone(uint8_t *start, uint8_t *end, size_t pages, heap_zone_type type);
		void *find_unused_allocation() const;
		void *find_allocation_for_pointer(void *ptr) const;
		kern_return_t use_allocation(void *&outPointer, void *allocation, size_t size);
		kern_return_t free_allocation(void *allocation);

		heap_zone_type _type;
		uint32_t _changes;

		uint8_t *_begin;
		uint8_t *_end;

		size_t _pages;
		size_t _freeSize;

		size_t _maxAllocations;
		size_t _freeAllocations;
		size_t _allocations;

		uint8_t *_firstAllocation;
		uint8_t *_lastAllocation;
	};

	class heap
	{
	public:
		enum
		{
			flag_secure  = (1 << 0),
			flag_aligned = (1 << 1)
		};

		heap(uint32_t flags);
		~heap();

		void *operator new(size_t size);
		void operator delete(void *ptr);

		size_t get_zone_count() const { return _zones.get_count(); }

		void *allocate(size_t size);
		void free(void *ptr);

	private:
		void *allocate_from_zone(heap_zone *zone, size_t size);

		spinlock_t _lock;
		cpp::list<heap_zone> _zones;
		uint32_t _flags;
	};

	heap *get_generic_heap();
	kern_return_t heap_init();
}

#endif /* _HEAP_H_ */
