//
//  IORunLoop.cpp
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

#include "IORunLoop.h"
#include "IOThread.h"
#include "IOAutoreleasePool.h"
#include "IOLog.h"

#ifdef super
#undef super
#endif
#define super IOObject

IORegisterClass(IORunLoop, super);

extern "C" void sd_yield();

IORunLoop *IORunLoop::currentRunLoop()
{
	IOThread *thread = IOThread::currentThread();
	return thread ? thread->_runLoop : 0;
}


IORunLoop *IORunLoop::initWithThread(IOThread *thread)
{
	if(super::init())
	{
		_host = thread;
		_lock = KERN_SPINLOCK_INIT;

		_eventSources = IOArray::alloc()->init();
	}

	return this;
}

void IORunLoop::free()
{
	for(uint32_t i=0; i<_eventSources->count(); i++)
	{
		IOEventSource *source = (IOEventSource *)_eventSources->objectAtIndex(i);
		source->disable();
		source->_runLoop = 0;
	}

	_eventSources->release();

	super::free();
}

bool IORunLoop::isOnThread()
{
	IOThread *thread = IOThread::currentThread();
	return (thread == _host);
}

// ----
// Event sources
// ----

void IORunLoop::addEventSource(IOEventSource *eventSource)
{
	_eventSources->addObject(eventSource);
	eventSource->setRunLoop(this);
}

void IORunLoop::removeEventSource(IOEventSource *eventSource)
{
	eventSource->setRunLoop(0);
	_eventSources->removeObject(eventSource);
}

void IORunLoop::processEventSources()
{
	for(size_t i=0; i<_eventSources->count(); i++)
	{
		IOEventSource *eventSource = (IOEventSource *)_eventSources->objectAtIndex(i);

		if(eventSource->isEnabled())
			eventSource->doWork();
	}
}

void IORunLoop::signalWorkAvailable()
{
	_host->wakeup();
}

// ----
// Run loop misc
// ----

void IORunLoop::step()
{
	IOAutoreleasePool *pool = IOAutoreleasePool::alloc()->init();
	kern_spinlock_lock(&_lock);

	processEventSources();


	kern_spinlock_unlock(&_lock);
	pool->release();
}

void IORunLoop::run()
{
	_shouldStop = false;
	while(!_shouldStop)
	{
		step();
		_host->sleep(10);
	}
}

void IORunLoop::stop()
{
	_shouldStop = true;
	_host->wakeup();
}
