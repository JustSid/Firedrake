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

#ifdef super
#undef super
#endif
#define super IOObject

IORegisterClass(IORunLoop, super);

extern "C" void sd_yield();

IORunLoop *IORunLoop::currentRunLoop()
{
	IOThread *thread = IOThread::currentThread();
	return thread->_runLoop;
}


IORunLoop *IORunLoop::init()
{
	if(super::init())
	{
		_shouldStop = false;
		_lock = IO_SPINLOCK_INIT;
	}

	return this;
}

void IORunLoop::free()
{
	super::free();
}



void IORunLoop::step()
{
	IOAutoreleasePool *pool = IOAutoreleasePool::alloc();
	pool->init();


	pool->release();
}

void IORunLoop::run()
{
	_shouldStop = false;
	while(!_shouldStop)
	{
		step();
		sd_yield();
	}
}

void IORunLoop::stop()
{
	_shouldStop = true;
}
