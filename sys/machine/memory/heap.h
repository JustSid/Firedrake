//
//  heap.h
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

#ifndef _HEAP_H_
#define _HEAP_H_

#include <prefix.h>
#include <libc/stddef.h>
#include <libc/stdint.h>
#include <libcpp/list.h>
#include <libcpp/bitfield.h>
#include <kern/kern_return.h>
#include <libc/sys/spinlock.h>

namespace Sys
{
	enum class HeapZoneType
	{
		Tiny, // Less than 64 bytes
		Small, // Less than 256
		Medium, // Less than 2048
		Large // Everything else
	};

	enum class HeapZoneAllocationType : uint8_t
	{
		Free,
		Used,
		Unused
	};

	class HeapZone;
	class Heap;

	class AllocationProxy
	{
	public:
		friend class HeapZone;

		AllocationProxy() {}

		HeapZoneAllocationType GetType() const;
		size_t GetSize() const;
		void *GetPointer() const;

		KernReturn<void> Free();
		KernReturn<void *> UseWithSize(size_t size);

	private:
		AllocationProxy(void *allocation, const HeapZone *zone);

		HeapZone *_zone;
		void *_allocation;
		bool _tiny;
	};

	class HeapZone : public cpp::list<HeapZone>::node
	{
	public:
		friend class AllocationProxy;
		friend class Heap;

		static KernReturn<HeapZone *> Create(size_t size);
		void operator delete(void *ptr);

		bool IsTiny() const;
		bool IsEmpty() const { return _allocations == _freeAllocations; }
		bool CanAllocate(size_t size) const;
		bool ContainsAllocation(void *ptr) const;

		size_t GetFreeSize() const { return _freeSize; }
		HeapZoneType GetType() const { return _type; }

		void Defragment();

	private:
		HeapZone(uint8_t *start, uint8_t *end, size_t pages, HeapZoneType type);

		bool FreeAllocationForSize(AllocationProxy &proxy, size_t size) const;
		bool AllocationForPointer(AllocationProxy &proxy, void *ptr) const;
		bool UnusedAllocation(AllocationProxy &proxy) const;

		void *FindUnusedAllocation() const;
		void *FindAllocationForPointer(void *ptr) const;
		KernReturn<void *> UseAllocation(void *allocation, size_t size);
		KernReturn<void> FreeAllocation(void *allocation);

		HeapZoneType _type;
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

	class Heap
	{
	public:
		struct Flags : cpp::bitfield<uint32_t>
		{
			Flags() = default;
			Flags(int value) :
				bitfield(value)
			{}

			enum
			{
				Secure  = (1 << 0),
				Aligned = (1 << 1)
			};
		};

		Heap(Flags flags);
		~Heap();

		static Heap *GetGenericHeap();

		void *operator new(size_t size);
		void operator delete(void *ptr);

		size_t GetZoneCount() const { return _zones.get_count(); }

		void *Allocate(size_t size);
		void Free(void *ptr);

	private:
		void *AllocateFromZone(HeapZone *zone, size_t size);

		spinlock_t _lock;
		cpp::list<HeapZone> _zones;
		Flags _flags;
	};

	KernReturn<void> HeapInit();
}

#endif /* _HEAP_H_ */
