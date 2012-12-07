//
//  IOEventSource.cpp
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

#include "IOEventSource.h"
#include "IORunLoop.h"

#ifdef super
#undef super
#endif
#define super IOObject

IORegisterClass(IOEventSource, super);

IOEventSource *IOEventSource::init()
{
	if(super::init())
	{
	}

	return this;
}

IOEventSource *IOEventSource::initWithAction(IOObject *owner, Action action)
{
	if(init())
	{
		_action = action;
		_owner = owner;
	}

	return this;
}


void IOEventSource::free()
{
	if(_runLoop)
		_runLoop->removeEventSource(this);

	super::free();
}


void IOEventSource::setRunLoop(IORunLoop *runLoop)
{
	if(!runLoop)
		disable();

	_runLoop = runLoop;

	if(_runLoop && !_enabled)
		enable();
}
void IOEventSource::signalWorkAvailable()
{
	if(_runLoop && _enabled)
		_runLoop->signalWorkAvailable();
}


void IOEventSource::doWork()
{
}

void IOEventSource::setAction(IOObject *owner, Action action)
{
	_owner = owner;
	_action = action;
}

void IOEventSource::enable()
{
	_enabled = true;
}

void IOEventSource::disable()
{
	_enabled = false;
}

bool IOEventSource::isEnabled()
{
	return _enabled;
}
