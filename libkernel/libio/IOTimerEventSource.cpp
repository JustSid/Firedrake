//
//  IOTimerEventSource.cpp
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

#include "IOTimerEventSource.h"
#include "IORunLoop.h"

#ifdef super
#undef super
#endif
#define super IOEventSource

IORegisterClass(IOTimerEventSource, super);

IOTimerEventSource *IOTimerEventSource::initWithDate(IODate *firedate, bool repeats)
{
	if(!super::init())
		return 0;

	timestamp_t current = time_getTimestamp();

	_fireDate = firedate->timestamp();
	_fireInterval = _fireDate - current;
	_repeats = repeats;

	return this;
}

IOTimerEventSource *IOTimerEventSource::initWithDate(IODate *firedate, IOObject *owner, IOTimerEventSource::Action action, bool repeats)
{
	if(!super::initWithAction(owner, (IOEventSource::Action)action))
		return 0;

	timestamp_t current = time_getTimestamp();

	_fireDate = firedate->timestamp();
	_fireInterval = _fireDate - current;
	_repeats = repeats;

	return this;
}

void IOTimerEventSource::doWork()
{
	timestamp_t current = time_getTimestamp();

	if(current > _fireDate)
	{
		IOTimerEventSource::Action action = (IOTimerEventSource::Action)_action;
		action(_owner, this);

		if(!_repeats)
		{
			disable();
			_runLoop->removeEventSource(this);

			return;
		}

		_fireDate = current + _fireInterval;
	}
}

IODate *IOTimerEventSource::fireDate() const
{
	IODate *date = IODate::alloc()->initWithTimestamp(_fireDate);
	return date->autorelease();
}
