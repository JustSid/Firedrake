//
//  IORemoteCommand.cpp
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

#include "IORemoteCommand.h"

#ifdef super
#undef super
#endif
#define super IOEventSource

IORegisterClass(IORemoteCommand, super);

IORemoteCommand *IORemoteCommand::init()
{
	return (IORemoteCommand *)super::init();
}

IORemoteCommand *IORemoteCommand::initWithAction(IOObject *owner, Action action)
{
	return (IORemoteCommand *)super::initWithAction(owner, action);
}

void IORemoteCommand::doWork()
{
	kern_spinlock_lock(&_lock);

	if(!_waiting || _cancelled)
	{
		kern_spinlock_unlock(&_lock);
		return;
	}

	_executing = true;
	kern_spinlock_unlock(&_lock);

	_action(_owner, _arg0, _arg1, _arg2, _arg3, _arg4);

	kern_spinlock_lock(&_lock);
	_executing = false;
	_executed = true;
	_waiting = false;
	kern_spinlock_unlock(&_lock);

	_caller->wakeup();
}

IOReturn IORemoteCommand::executeAction(Action action, void *arg0, void *arg1, void *arg2, void *arg3, void *arg4)
{
	Action temp = _action;
	_action = action;

	IOReturn result = executeCommand(arg0, arg1, arg2, arg3, arg4);

	_action = temp;
	return result;
}

IOReturn IORemoteCommand::executeCommand(void *arg0, void *arg1, void *arg2, void *arg3, void *arg4)
{
	if(_runLoop->isOnThread())
	{
		_action(_owner, _arg0, _arg1, _arg2, _arg3, _arg4);
		return kIOReturnSuccess;
	}

	kern_spinlock_lock(&_lock);

	_cancelled = false;
	_waiting = true;
	_caller = IOThread::currentThread();

	_arg0 = arg0;
	_arg1 = arg1;
	_arg2 = arg2;
	_arg3 = arg3;
	_arg4 = arg4;

	kern_spinlock_unlock(&_lock);

	uint64_t timeout = (_timeout > 0) ? _timeout : 10;
	while(1)
	{
		_caller->sleep(timeout);

		kern_spinlock_lock(&_lock);

		if(_executed)
		{
			kern_spinlock_unlock(&_lock);
			return kIOReturnSuccess;
		}

		if(_timeout > 0 && !_executing)
		{
			_cancelled = true;
			kern_spinlock_unlock(&_lock);
			return kIOReturnTimeout;
		}

		kern_spinlock_unlock(&_lock);
	}
}
