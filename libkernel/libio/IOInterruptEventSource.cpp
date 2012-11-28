//
//  IOInterruptEventSource.cpp
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

#include <libkernel/base.h>
#include <libkernel/interrupts.h>
#include "IOInterruptEventSource.h"
#include "IOLog.h"

#ifdef super
#undef super
#endif
#define super IOEventSource

IORegisterClass(IOInterruptEventSource, super);

IOInterruptEventSource *IOInterruptEventSource::initWithInterrupt(uint32_t interrupt, bool exclusive)
{
	if(!super::init())
		return 0;

	_interrupt = interrupt;
	_exclusive = exclusive;

	bool result = registerForInterrupt();
	if(!result)
	{
		release();
		return 0;
	}

	return this;
}

IOInterruptEventSource *IOInterruptEventSource::initWithInterrupt(uint32_t interrupt, IOObject *owner, IOInterruptEventSource::Action action, bool exclusive)
{
	if(!super::initWithAction(owner, (IOEventSource::Action)action))
		return 0;

	_interrupt = interrupt;
	_exclusive = exclusive;

	bool result = registerForInterrupt();
	if(!result)
	{
		release();
		return 0;
	}

	return this;
}

void IOInterruptEventSource::free()
{
	if(_enabled)
	{
		kern_registerForInterrupt(_interrupt, false, this, this, 0);
		_enabled = false;
	}

	super::free();
}

bool IOInterruptEventSource::registerForInterrupt()
{
	IOReturn result;
	kern_interrupt_handler_t callback = IOMemberFunctionCast(kern_interrupt_handler_t, this, &IOInterruptEventSource::handleInterrupt);

	if(_interrupt == 0)
	{
		result = kern_requestExclusiveInterrupt(_owner, this, &_interrupt, callback);
	}
	else
	{
		result = kern_registerForInterrupt(_interrupt, _exclusive, this, this, callback);
	}

	return (result == kIOReturnSuccess);
}

void IOInterruptEventSource::doWork()
{
	uint32_t tcount = _count;
	uint32_t missed = tcount - _consumed;	

	if(missed > 0 && _action)
	{
		IOInterruptEventSource::Action action = (IOInterruptEventSource::Action)_action;
		action(_owner, this, missed);

		_consumed = tcount;
	}
}

void IOInterruptEventSource::handleInterrupt()
{
	_count ++;
	signalWorkAvailable();
}
