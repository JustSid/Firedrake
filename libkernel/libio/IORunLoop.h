//
//  IORunLoop.h
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

#ifndef _IORUNLOOP_H_
#define _IORUNLOOP_H_

#include <libkernel/spinlock.h>

#include "IOObject.h"
#include "IOArray.h"
#include "IOSet.h"
#include "IOEventSource.h"

class IOThread;

class IORunLoop : public IOObject
{
friend class IOThread;
friend class IOEventSource;
public:
	static IORunLoop *currentRunLoop();	

	bool isOnThread();

	void run();
	void stop();

	void addEventSource(IOEventSource *eventSource);
	void removeEventSource(IOEventSource *eventSource);

protected:
	void step();
	void signalWorkAvailable();

private:
	virtual IORunLoop *initWithThread(IOThread *thread);
	virtual void free();

	void processEventSources();

	IOThread *_host;
	IOArray *_eventSources;
	IOSet *_removedSources;

	bool _shouldStop;
	kern_spinlock_t _lock;

	IODeclareClass(IORunLoop)
};

#endif /* _IORUNLOOP_H_ */
