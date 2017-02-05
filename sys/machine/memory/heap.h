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

#include <libc/stdint.h>
#include <libc/sys/spinlock.h>
#include <kern/kern_return.h>

namespace Sys
{
	class Heap
	{
	public:

		Heap();
		~Heap();

		void *operator new(size_t size);
		void operator delete(void *ptr);

		static Heap *GetGenericHeap();
		static void SwitchToPanicHeap();

		void *Allocate(size_t size, size_t alignment = 4);
		void Free(void *pointer);

	private:
		struct __Allocation
		{
			enum class Type : uint8_t
			{
				Free,
				Allocated,
				Unused
			};

			Type type;
			size_t size;
			size_t padding;
			uint8_t *pointer;
		};

		class Arena
		{
		public:
			enum class Type : uint8_t
			{
				Tiny = 0,
				Small = 1,
				Medium = 2,
				Large = 3
			};

			static Type GetTypeForSize(size_t size);

			Arena(Type type, size_t sizeHint);
			~Arena();

			void *operator new(size_t size);
			void operator delete(void *ptr);

			bool CanAllocate(size_t size);
			bool ContainsAllocation(void *pointer);
			bool IsEmpty() const { return (_allocations == _freeAllocations); }

			void *Allocate(size_t size, size_t alignment);
			void Free(void *pointer);

			void Defragment();

			Arena *next;
			Arena *prev;

		private:
			__Allocation *FindFreeAllocation(size_t size, size_t alignment);
			__Allocation *GetAllocationForPointer(void *pointer);

			size_t _changes;

			uint8_t *_begin;
			uint8_t *_end;

			__Allocation *_allocationStart;
			__Allocation *_allocationEnd;
			__Allocation *_cacheAllocation;

			size_t _pages;
			size_t _allocationPages;

			size_t _freeBytes;
			size_t _allocations;
			size_t _freeAllocations;
		};

		Arena *_arenas[4]; // Linked list for every arena type
		spinlock_t _lock;
	};

	KernReturn<void> HeapInit();
}

#endif
