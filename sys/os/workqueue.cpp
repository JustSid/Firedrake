//
//  workqueue.cpp
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

#include "workqueue.h"

namespace OS
{
	WorkQueue::WorkQueue() :
		_freeListHead(nullptr),
		_workListHead(nullptr),
		_exhausted(false)
	{
		spinlock_init(&_readWriterLock);
		_ExtendFreeList(50);
	}

	void WorkQueue::_ExtendFreeList(size_t size)
	{
		Entry *entries = new Entry[size];

		for(size_t i = 0; i < size - 1; i ++)
		{
			Entry *entry = entries + i;
			Entry *next = entries + i + 1;

			entry->next = next;
			next->next = nullptr;
		}

		RefurbishList(entries);
	}

	bool WorkQueue::PushEntry(Callback callback, void *context)
	{
		spinlock_lock(&_readWriterLock);

		Entry *entry = _freeListHead;
		if(!entry)
		{
			spinlock_unlock(&_readWriterLock);
			return false;
		}

		_freeListHead = entry->next;

		entry->callback = callback;
		entry->context = context;

		entry->next = _workListHead;
		_workListHead = entry;

		spinlock_unlock(&_readWriterLock);
		return true;
	}

	WorkQueue::Entry *WorkQueue::PopAll()
	{
		Entry *entry = _workListHead;
		_workListHead = nullptr;

		// Pop All is not safe to call from an interrupt context
		// Which means it is safe for us to extend the free list
		if(_exhausted)
		{
			_ExtendFreeList(10);
			_exhausted = false;
		}

		return entry;
	}

	WorkQueue::Entry *WorkQueue::PopAllRemote()
	{
		spinlock_lock(&_readWriterLock);
		Entry *result = PopAll();
		spinlock_unlock(&_readWriterLock);

		return result;
	}

	void WorkQueue::RefurbishList(Entry *entry)
	{
		if(!entry->next)
		{		
			entry->next = _freeListHead;
			_freeListHead = entry;
		}
		else
		{
			Entry *last = entry;
			do {
				last = last->next;
			} while(last->next);
			
			last->next = _freeListHead;
			_freeListHead = entry;
		}
	}

	void WorkQueue::RefurbishListRemote(Entry *entry)
	{
		spinlock_lock(&_readWriterLock);
		RefurbishList(entry);
		spinlock_unlock(&_readWriterLock);
	}
}
