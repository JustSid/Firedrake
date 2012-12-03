//
//  IODate.cpp
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

#include "IODate.h"

#ifdef super
#undef super
#endif
#define super IOObject

IORegisterClass(IODate, super);

IODate *IODate::init()
{
	if(!super::init())
		return 0;

	_delta = time_getTimestamp();
	return this;
}

IODate *IODate::initWithTimestamp(timestamp_t time)
{
	if(!super::init())
		return 0;

	_delta = time;
	return this;
}

IODate *IODate::initWithTimestampSinceNow(timestamp_t time)
{
	if(!super::init())
		return 0;

	_delta = time_getTimestamp() + time;
	return this;
}


bool IODate::isEqual(IOObject *other) const
{
	if(this == other)
		return true;

	if(other->isSubclassOf(symbol()))
		return false;

	IODate *date = (IODate *)other;
	return _delta == date->_delta;
}

hash_t IODate::hash() const
{
	return (hash_t)_delta;
}

unix_time_t IODate::date() const
{
	unix_time_t delta = time_getBootTime();
	return delta + time_convertTimestamp(_delta);
}

unix_time_t IODate::unixTimestamp() const
{
	return time_convertTimestamp(_delta);
}

timestamp_t IODate::timestamp() const
{
	return _delta;
}
