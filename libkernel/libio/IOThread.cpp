//
//  IOThread.cpp
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

#include "IOThread.h"
#include "IOAutoreleasePool.h"
#include "IORunLoop.h"
#include "IODictionary.h"
#include "IORuntime.h"
#include "IONumber.h"
#include "IOLog.h"

#ifdef super
#undef super
#endif
#define super IOObject

IORegisterClass(IOThread, super);

extern "C"
{
	thread_t __io_threadCreate(void *entry, void *owner, void *arg);
	thread_t __io_threadID();
	void __io_threadSetName(thread_t id, const char *name);
	void __io_threadSleep(thread_t id, uint64_t time);
	void __io_threadWakeup(thread_t id);
	void sd_yield();
}

static kern_spinlock_t __IOThreadLock = KERN_SPINLOCK_INIT;
static IODictionary *__IOThreadDictionary = 0;


IOThread *IOThread::initWithFunction(IOThread::Function function)
{
	if(super::init())
	{
		_detached = false;
		_entry    = function;
		_lock     = KERN_SPINLOCK_INIT;
	}

	return this;
}

void IOThread::free()
{
	_properties->release();
	_topPool->release();
	_name->release();

	super::free();
}


IOThread *IOThread::withFunction(IOThread::Function function)
{
	IOThread *thread = IOThread::alloc()->initWithFunction(function);
	if(!thread)
		return 0;

	thread->detach();

	return thread->autorelease();
}

IOThread *IOThread::currentThread()
{
	IOThread *thread = 0;
	IONumber *threadID = IONumber::alloc()->initWithUInt32(__io_threadID());

	kern_spinlock_lock(&__IOThreadLock);
	if(__IOThreadDictionary)
		thread = (IOThread *)__IOThreadDictionary->objectForKey(threadID);
	kern_spinlock_unlock(&__IOThreadLock);

	threadID->release();
	return thread;
}

void IOThread::yield()
{
	sd_yield();
}


void IOThread::threadEntry()
{
	IONumber *threadID = IONumber::alloc()->initWithUInt32(_id);

	kern_spinlock_lock(&_lock);
	kern_spinlock_lock(&__IOThreadLock);
	{
		if(!__IOThreadDictionary)
		{
			__IOThreadDictionary = IODictionary::alloc()->initWithCapacity(5);
			if(!__IOThreadDictionary)
				panic("Couldn't create __IOThreadDictionary!");
		}
		
		__IOThreadDictionary->setObjectForKey(this, threadID);
	}
	kern_spinlock_unlock(&__IOThreadLock);
	kern_spinlock_unlock(&_lock);

	// Create a run loop for the thread
	_runLoop = IORunLoop::alloc()->initWithThread(this);
	_entry(this);
	_runLoop->release();

	kern_spinlock_lock(&_lock);
	kern_spinlock_lock(&__IOThreadLock);
	{
		__IOThreadDictionary->removeObjectForKey(threadID);
		threadID->release();
	}
	kern_spinlock_unlock(&__IOThreadLock);
	release();
}

void IOThread::sleep(uint64_t time)
{
	__io_threadSleep(_id, time);
}

void IOThread::wakeup()
{
	__io_threadWakeup(_id);
}

void IOThread::detach()
{
	kern_spinlock_lock(&_lock);

	if(!_detached)
	{
		retain();

		_detached = true;
		_id = __io_threadCreate(IOMemberFunctionCast(void *, this, &IOThread::threadEntry), this, 0);
	}

	kern_spinlock_unlock(&_lock);
}

void IOThread::setName(IOString *name)
{
	kern_spinlock_lock(&_lock);

	__io_threadSetName(_id, name->CString());
	_name->release();
	_name = name->retain();

	kern_spinlock_unlock(&_lock);
}

void IOThread::setPropertyForKey(IOObject *property, IOObject *key)
{
	kern_spinlock_lock(&_lock);

	if(!_detached)
	{
		if(!_properties)
			_properties = IODictionary::alloc()->init();

		_properties->setObjectForKey(property, key);
	}

	kern_spinlock_unlock(&_lock);
}



IOString *IOThread::name()
{
	kern_spinlock_lock(&_lock);

	IOString *copy = 0;
	if(_name)
		copy = IOString::alloc()->initWithString(_name);

	kern_spinlock_unlock(&_lock);

	return copy->autorelease();
}

IORunLoop *IOThread::runLoop() const
{
	return _runLoop;
}

IOAutoreleasePool *IOThread::autoreleasePool() const
{
	return _topPool;
}

IOObject *IOThread::propertyForKey(IOObject *key)
{
	kern_spinlock_lock(&_lock);

	if(!_properties)
	{
		kern_spinlock_unlock(&_lock);
		return 0;
	}

	IOObject *property = _properties->objectForKey(key);
	kern_spinlock_unlock(&_lock);

	return property;
}
