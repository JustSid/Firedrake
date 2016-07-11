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

#include <kern/panic.h>
#include <kern/kprintf.h>
#include <libcpp/new.h>
#include "memory.h"
#include "heap.h"

namespace Sys
{
	constexpr size_t kHeapSafeZone = 4;

	Heap::Arena::Arena(Type type, size_t sizeHint) :
		next(nullptr),
		prev(nullptr),
		_changes(0),
		_begin(nullptr),
		_end(nullptr),
		_allocationStart(nullptr),
		_allocationEnd(nullptr),
		_cacheAllocation(nullptr),
		_allocations(0)
	{
		switch(type)
		{
			case Type::Tiny:
				_pages = 2;
				_allocationPages = 2;
				break;
			case Type::Small:
				_pages = 4;
				_allocationPages = 2;
				break;
			case Type::Medium:
				_pages = 10;
				_allocationPages = 1;
				break;
			case Type::Large:
				_pages = VM_PAGE_COUNT(sizeHint);
				_allocationPages = 1;
				break;
		}

		Sys::VM::Directory *directory = VM::Directory::GetKernelDirectory();

		_begin = Sys::Alloc<uint8_t>(directory, _pages, kVMFlagsKernel);
		_end = _begin + (_pages * VM_PAGE_SIZE);

		uint8_t *allocations = Sys::Alloc<uint8_t>(directory, _allocationPages, kVMFlagsKernel);
		uint8_t *allocationsEnd = allocations + ((VM_PAGE_SIZE * _allocationPages) / sizeof(__Allocation));

		_allocationStart = reinterpret_cast<__Allocation *>(allocations);
		_allocationEnd = reinterpret_cast<__Allocation *>(allocationsEnd);

		_allocations = 1;
		_freeAllocations = 1;
		_freeBytes = _end - _begin;

		__Allocation *allocation = _allocationStart;

		allocation->pointer = _begin;
		allocation->type = __Allocation::Type::Free;
		allocation->padding = 0;
		allocation->size = _freeBytes;

		for(allocation ++; allocation != _allocationEnd; allocation ++)
		{
			allocation->pointer = nullptr;
			allocation->type = __Allocation::Type::Unused;
		}
	}

	Heap::Arena::~Arena()
	{
		Sys::VM::Directory *directory = VM::Directory::GetKernelDirectory();

		if(_begin)
			Sys::Free(_begin, directory, _pages);
		if(_allocationStart)
			Sys::Free(_allocationStart, directory, _allocationPages);
	}

	void *Heap::Arena::operator new(__unused size_t size)
	{
		void *buffer = Alloc<void>(VM::Directory::GetKernelDirectory(), 1, kVMFlagsKernel);
		return buffer;
	}

	void Heap::Arena::operator delete(void *ptr)
	{
		Sys::Free(ptr, VM::Directory::GetKernelDirectory(), 1);
	}


	Heap::Arena::Type Heap::Arena::GetTypeForSize(size_t size)
	{
		if(size > 2048)
			return Type::Large;
		if(size > 256)
			return Type::Medium;
		if(size > 32)
			return Type::Small;

		return Type::Tiny;
	}

	bool Heap::Arena::CanAllocate(size_t size)
	{
		if(_freeBytes < size || _freeAllocations == 0)
			return false;

		__Allocation *allocation = _allocationStart;
		while(allocation != _allocationEnd)
		{
			if(allocation->type == __Allocation::Type::Free && allocation->size >= size)
			{
				_cacheAllocation = allocation;
				return true;
			}

			allocation ++;
		}

		return false;
	}

	bool Heap::Arena::ContainsAllocation(void *pointer)
	{
		if(pointer < _begin || pointer > _end)
			return false;

		__Allocation *allocation = _allocationStart;
		while(allocation != _allocationEnd)
		{
			if(allocation->type == __Allocation::Type::Allocated && allocation->pointer == pointer)
			{
				_cacheAllocation = allocation;
				return true;
			}

			allocation ++;
		}

		return false;
	}

	Heap::__Allocation *Heap::Arena::GetAllocationForPointer(void *pointer)
	{
		if(_cacheAllocation && (_cacheAllocation->type == __Allocation::Type::Allocated && _cacheAllocation->pointer == pointer))
			return _cacheAllocation;

		__Allocation *allocation = _allocationStart;
		while(allocation != _allocationEnd)
		{
			if(allocation->type == __Allocation::Type::Allocated && allocation->pointer == pointer)
				return allocation;

			allocation ++;
		}

		return nullptr;
	}

	Heap::__Allocation *Heap::Arena::FindFreeAllocation(size_t size, size_t alignment)
	{
		if(_cacheAllocation && _cacheAllocation->type == __Allocation::Type::Free)
		{
			size_t padding = reinterpret_cast<size_t>(_cacheAllocation->pointer) % alignment;

			if(_cacheAllocation->size - padding >= size)
				return _cacheAllocation;
		}

		__Allocation *allocation = _allocationStart;
		while(allocation != _allocationEnd)
		{
			if(allocation->type == __Allocation::Type::Free)
			{
				size_t padding = reinterpret_cast<size_t>(allocation->pointer) % alignment;

				if(allocation->size - padding >= size)
					return allocation;
			}

			allocation ++;
		}

		return nullptr;
	}

	void *Heap::Arena::Allocate(size_t size, size_t alignment)
	{
		__Allocation *allocation = FindFreeAllocation(size, alignment);
		if(!allocation)
			return nullptr;

		size_t shift = reinterpret_cast<size_t>(allocation->pointer) % alignment;

		allocation->padding = shift;
		allocation->pointer += shift;
		allocation->size -= shift;

		allocation->type = __Allocation::Type::Allocated;

		uint8_t *result = allocation->pointer;

		size_t overflow = allocation->size - size;
		if(overflow > 4)
		{
			// Divide the allocation up into two
			__Allocation *temp = _allocationStart;

			while(temp != _allocationEnd)
			{
				if(temp->type == __Allocation::Type::Unused)
				{
					temp->type = __Allocation::Type::Free;
					temp->padding = 0;
					temp->pointer = result + size;
					temp->size = overflow;

					allocation->size -= overflow;

					_allocations ++;
					_freeAllocations ++;
					break;
				}

				temp ++;
			}
		}

		_freeBytes -= allocation->size;
		_freeAllocations --;

		return result;
	}

	void Heap::Arena::Free(void *pointer)
	{
		__Allocation *allocation = GetAllocationForPointer(pointer);
		if(!allocation)
			return;

		allocation->type = __Allocation::Type::Free;
		allocation->pointer -= allocation->padding;
		allocation->size += allocation->padding;
		allocation->padding = 0;

		_freeAllocations ++;
		_freeBytes += allocation->size;

		if((++ _changes) >= 15)
		{
			Defragment();
			_changes = 0;
		}
	}

	void Heap::Arena::Defragment()
	{
		// No-op for now
	}

	void Heap::Arena::Dump()
	{
		kprintf("Arena {%p:%p}\n", _begin, _end);

		__Allocation *temp = _allocationStart;

		while(temp != _allocationEnd)
		{
			switch(temp->type)
			{
				case __Allocation::Type::Free:
					kprintf("  Free: |-%p-%d-|\n", temp->pointer, temp->size);
					break;
				case __Allocation::Type::Allocated:
					kprintf("  Allocated: |-%d-%p-%d-|\n", temp->padding, temp->pointer, temp->size);
					break;

				default:
					break;
			}

			temp ++;
		}
	}


	// -----------------
	// Heap
	// -----------------

	Heap::Heap()
	{
		spinlock_init(&_lock);

		_arenas[0] = nullptr;
		_arenas[1] = nullptr;
		_arenas[2] = nullptr;
		_arenas[3] = nullptr;
	}

	Heap::~Heap()
	{}

	void *Heap::operator new(__unused size_t size)
	{
		void *buffer = Alloc<void>(VM::Directory::GetKernelDirectory(), 1, kVMFlagsKernel);
		return buffer;
	}

	void Heap::operator delete(void *ptr)
	{
		Sys::Free(ptr, VM::Directory::GetKernelDirectory(), 1);
	}

	void *Heap::Allocate(size_t size, size_t alignment)
	{
		size += kHeapSafeZone * sizeof(void *);

		Arena::Type type = Arena::GetTypeForSize(size);

		spinlock_lock(&_lock);

		void *result = nullptr;

		Arena *arena = _arenas[static_cast<uint8_t>(type)];
		while(arena)
		{
			if(arena->CanAllocate(size))
			{
				result = arena->Allocate(size, alignment);

				if(result)
					break;
			}

			arena = arena->next;
		}

		if(!result)
		{
			Arena *arena = new Arena(type, size);
			if(!arena)
			{
				spinlock_unlock(&_lock);
				return nullptr;
			}
			
			Arena *next = _arenas[static_cast<uint8_t>(type)];

			arena->next = next;
			if(next)
				next->prev = arena;

			_arenas[static_cast<uint8_t>(type)] = arena;

			result = arena->Allocate(size, alignment);
		}

		spinlock_unlock(&_lock);

		return result;
	}

	void Heap::Free(void *pointer)
	{
		spinlock_lock(&_lock);

		bool foundAllocation = false;

		for(int i = 0; i < 4; i ++)
		{
			Arena *arena = _arenas[i];
			while(arena)
			{
				if(arena->ContainsAllocation(pointer))
				{
					arena->Free(pointer);

					if(arena->IsEmpty())
					{
						if(arena->next)
							arena->next->prev = arena->prev;
						if(arena->prev)
							arena->prev->next = arena->next;

						if(_arenas[i] == arena)
							_arenas[i] = arena->next;

						delete arena;
					}

					foundAllocation = true;
					break;
				}

				arena = arena->next;
			}

			if(foundAllocation)
				break;
		}

		spinlock_unlock(&_lock);

		if(!foundAllocation)
			panic("Tried to free unknown pointer %p!", pointer);
	}

	void Heap::Dump()
	{
		for(int i = 0; i < 4; i ++)
		{
			switch(i)
			{
				case 0:
				{
					kprintf("Tiny arenas:\n");

					Arena *arena = _arenas[i];
					while(arena)
					{
						arena->Dump();
						arena = arena->next;
					}

					break;
				}
				case 1:
				{
					kprintf("Small arenas:\n");

					Arena *arena = _arenas[i];
					while(arena)
					{
						arena->Dump();
						arena = arena->next;
					}

					break;
				}
				case 2:
				{
					kprintf("Medium arenas:\n");

					Arena *arena = _arenas[i];
					while(arena)
					{
						arena->Dump();
						arena = arena->next;
					}

					break;
				}
				case 3:
				{
					kprintf("Large arenas:\n");

					Arena *arena = _arenas[i];
					while(arena)
					{
						arena->Dump();
						arena = arena->next;
					}

					break;
				}
			}
		}
	}

	static Heap *_genericHeap;
	static Heap *_panicHeap;
	static bool _usePanicHeap = false;

	Heap *Heap::GetGenericHeap()
	{
		return __expect_false(_usePanicHeap) ? _panicHeap : _genericHeap;
	}

	void Heap::SwitchToPanicHeap()
	{
		_usePanicHeap = true;
	}

	KernReturn<void> HeapInit()
	{
		_genericHeap = new Heap();
		_panicHeap = new Heap();

		return (_genericHeap && _panicHeap) ? Error(KERN_SUCCESS) : Error(KERN_NO_MEMORY);
	}
}
