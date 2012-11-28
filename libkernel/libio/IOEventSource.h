//
//  IOEventSource.h
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

#ifndef _IOEVENTSOURCE_H_
#define _IOEVENTSOURCE_H_

#include "IOObject.h"

class IORunLoop;
class IOEventSource : public IOObject
{
friend class IORunLoop;
public:
	typedef void (*Action)(IOObject *owner, ...);

	virtual void doWork();

	virtual void setRunLoop(IORunLoop *runLoop);
	virtual void signalWorkAvailable();
	
	virtual void enable();
	virtual void disable();

	bool isEnabled();

protected:
	virtual IOEventSource *init();
	virtual IOEventSource *initWithAction(IOObject *owner, Action action);

	virtual void free();

	bool _enabled;

	IOObject *_owner;
	Action _action;

	IORunLoop *_runLoop;

	IODeclareClass(IOEventSource)
};

#endif /* _IOEVENTSOURCE_H_ */
