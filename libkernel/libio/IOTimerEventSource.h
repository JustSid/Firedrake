//
//  IOTimerEventSource.h
//  libio
//
//  Created by Sidney Just
//  Copyright (c) 2013 by Sidney Just
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

#ifndef _IOTIMEREVENTSOURCE_H_
#define _IOTIMEREVENTSOURCE_H_

#include "IOObject.h"
#include "IOEventSource.h"
#include "IODate.h"

class IOTimerEventSource : public IOEventSource
{
friend class IORunLoop;
public:
	typedef void (*Action)(IOObject *owner, IOTimerEventSource *source);

	IOTimerEventSource *initWithDate(IODate *firedate, bool repeats);
	IOTimerEventSource *initWithDate(IODate *firedate, IOObject *owner, IOTimerEventSource::Action action, bool repeats);

	virtual void doWork();
	virtual void setAction(IOObject *owner, Action action);

	IODate *fireDate() const;

private:
	bool _repeats;
	timestamp_t _fireInterval;
	timestamp_t _fireDate;

	IOObject *_object;

	IODeclareClass(IOTimerEventSource)
};

#endif /* _IOTIMEREVENTSOURCE_H_ */
