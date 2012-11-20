//
//  IOInterruptEventSource.h
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

#ifndef _IOINTERRUPTEVENTSOURCE_H_
#define _IOINTERRUPTEVENTSOURCE_H_

#include "IOEventSource.h"

class IOInterruptEventSource : public IOEventSource
{
public:
	typedef void (*Action)(IOObject *owner, IOInterruptEventSource *source, uint32_t missed);

	IOInterruptEventSource *initWithInterrupt(uint32_t interrupt, bool exclusive=false);
	IOInterruptEventSource *initWithInterrupt(uint32_t interrupt, IOObject *owner, IOInterruptEventSource::Action action, bool exclusive=false);

	virtual void doWork();

	uint32_t interrupt() { return _interrupt; }

private:
	virtual void free();

	bool registerForInterrupt();
	void handleInterrupt();

	uint32_t _interrupt;
	bool _exclusive;

	uint32_t _count;
	uint32_t _consumed;
	
	IODeclareClass(IOInterruptEventSource)
};

#endif /* _IOINTERRUPTEVENTSOURCE_H_ */
