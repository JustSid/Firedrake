//
//  IORemoteCommand.h
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

#ifndef _IOREMOTECOMMAND_H_
#define _IOREMOTECOMMAND_H_

#include "IOObject.h"
#include "IOEventSource.h"
#include "IOThread.h"

class IORemoteCommand : public IOEventSource
{
public:
	virtual IORemoteCommand *init();
	virtual IORemoteCommand *initWithAction(IOObject *owner, Action action);
	
	virtual void doWork();

	void setTimeout(uint64_t timeout);
	uint64_t timeout();

	IOReturn executeAction(Action action, void *arg0=0, void *arg1=0, void *arg2=0, void *arg3=0, void *arg4=0);
	IOReturn executeCommand(void *arg0=0, void *arg1=0, void *arg2=0, void *arg3=0, void *arg4=0);

private:
	uint64_t _timeout;
	IOThread *_caller;

	kern_spinlock_t _lock;
	bool _waiting;
	bool _executed;
	bool _cancelled;
	bool _executing;

	void *_arg0;
	void *_arg1;
	void *_arg2;
	void *_arg3;
	void *_arg4;

	IODeclareClass(IORemoteCommand)
};

#endif /* _IOREMOTECOMMAND_H_ */
