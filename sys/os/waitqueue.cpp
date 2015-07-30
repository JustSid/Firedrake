//
//  waitqueue.cpp
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

#include <libio/IOObject.h>
#include <libio/IODictionary.h>
#include <libcpp/vector.h>
#include <os/scheduler/scheduler.h>
#include "waitqueue.h"

namespace OS
{
	class WaitqueueLookup : public IO::Object
	{
	public:
		WaitqueueLookup *Init(void *channel)
		{
			if(!IO::Object::Init())
				return nullptr;

			_channel = channel;

			return this;
		}

		size_t GetHash() const override
		{
			return reinterpret_cast<size_t>(_channel);
		}
		bool IsEqual(IO::Object *other) const override
		{
			WaitqueueLookup *lookup = static_cast<WaitqueueLookup *>(other);
			return (lookup->_channel == _channel);
		}

		void AddThread(Thread *thread)
		{
			_waiters.push_back(thread);
		}
		std::vector<Thread *>::iterator GetBegin()
		{
			return _waiters.begin();
		}
		std::vector<Thread *>::iterator GetEnd()
		{
			return _waiters.end();
		}

	private:
		std::vector<Thread *> _waiters;
		void *_channel;

		IODeclareMeta(WaitqueueLookup)
	};

	class WaitqueueEntry : public IO::Object
	{
	public:
		WaitqueueEntry *Init()
		{
			if(!IO::Object::Init())
				return nullptr;
			return this;
		}

		void AddThread(Thread *thread)
		{
			_waiters.push_back(thread);
		}
		std::vector<Thread *>::iterator GetBegin()
		{
			return _waiters.begin();
		}
		std::vector<Thread *>::iterator GetEnd()
		{
			return _waiters.end();
		}

		Thread *PopAny()
		{
			std::vector<Thread *>::iterator last = _waiters.end() - 1;

			Thread *thread = *last;
			_waiters.erase(last);

			return thread;
		}
		bool IsEmpty() const
		{
			return (_waiters.size() == 0);
		}

	private:
		std::vector<Thread *> _waiters;

		IODeclareMeta(WaitqueueEntry)
	};

	IODefineMeta(WaitqueueLookup, IO::Object)
	IODefineMeta(WaitqueueEntry, IO::Object)

	static IO::Dictionary *_waitqueue;
	static spinlock_t _waitLock = SPINLOCK_INIT;


	KernReturn<void> Wait(void *channel)
	{
		return WaitWithCallback(channel, []{});
	}

	KernReturn<void> WaitWithCallback(void *channel, IO::Function<void ()> &&callback)
	{
		if(!Sys::CPU::GetCurrentCPU()->GetFlagsSet(Sys::CPU::Flags::WaitQueueEnabled))
			return Error(KERN_RESOURCES_MISSING);

		WaitqueueLookup *lookup = WaitqueueLookup::Alloc()->Init(channel);

		spinlock_lock(&_waitLock);

		WaitqueueEntry *entry = _waitqueue->GetObjectForKey<WaitqueueEntry>(lookup);
		if(!entry)
		{
			entry = WaitqueueEntry::Alloc()->Init();
			_waitqueue->SetObjectForKey(entry, lookup);
		}

		Thread *thread = Scheduler::GetScheduler()->GetActiveThread();

		Scheduler::GetScheduler()->BlockThread(thread);
		entry->AddThread(thread);

		spinlock_unlock(&_waitLock);

		lookup->Release();

		callback();
		Scheduler::GetScheduler()->RescheduleCPU(Sys::CPU::GetCurrentCPU()); // Make sure we don't return until Wakeup() is called

		return ErrorNone;
	}

	KernReturn<void> WaitThread(Thread *thread, void *channel)
	{
		IO::StrongRef<WaitqueueLookup> lookup(IOTransferRef(WaitqueueLookup::Alloc()->Init(channel)));

		spinlock_lock(&_waitLock);

		WaitqueueEntry *entry = _waitqueue->GetObjectForKey<WaitqueueEntry>(lookup);
		if(!entry)
		{
			entry = WaitqueueEntry::Alloc()->Init();
			_waitqueue->SetObjectForKey(entry, lookup);
		}
		
		Scheduler::GetScheduler()->BlockThread(thread);
		entry->AddThread(thread);

		spinlock_unlock(&_waitLock);

		return ErrorNone;
	}
	
	void Wakeup(void *channel)
	{
		IO::StrongRef<WaitqueueLookup> lookup(IOTransferRef(WaitqueueLookup::Alloc()->Init(channel)));

		spinlock_lock(&_waitLock);

		WaitqueueEntry *entry = _waitqueue->GetObjectForKey<WaitqueueEntry>(lookup);
		if(!entry)
		{
			spinlock_unlock(&_waitLock);
			return;
		}

		entry->Retain();

		_waitqueue->RemoveObjectForKey(lookup);
		spinlock_unlock(&_waitLock);

		// Unblock all threads waiting on the channel
		std::vector<Thread *>::iterator iterator = entry->GetBegin();
		while(iterator != entry->GetEnd())
		{
			Thread *thread = *iterator;
			Scheduler::GetScheduler()->UnblockThread(thread);

			iterator ++;
		}

		entry->Release();
	}

	void WakeupOne(void *channel)
	{
		IO::StrongRef<WaitqueueLookup> lookup(IOTransferRef(WaitqueueLookup::Alloc()->Init(channel)));

		spinlock_lock(&_waitLock);

		WaitqueueEntry *entry = _waitqueue->GetObjectForKey<WaitqueueEntry>(lookup);
		if(!entry)
		{
			spinlock_unlock(&_waitLock);
			return;
		}

		Thread *thread = entry->PopAny();

		if(entry->IsEmpty())
			_waitqueue->RemoveObjectForKey(lookup);

		spinlock_unlock(&_waitLock);

		Scheduler::GetScheduler()->UnblockThread(thread);
	}

	KernReturn<void> WaitqueueInit()
	{
		_waitqueue = IO::Dictionary::Alloc()->Init();
		return ErrorNone;
	}
}
