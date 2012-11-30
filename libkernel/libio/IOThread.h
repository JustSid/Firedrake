//
//  IOThread.h
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

#ifndef _IOTHREAD_H_
#define _IOTHREAD_H_

#include <libkernel/spinlock.h>
#include <libkernel/thread.h>

#include "IOTypes.h"
#include "IOObject.h"
#include "IORunLoop.h"
#include "IODictionary.h"
#include "IOString.h"

class IOAutoreleasePool;

class IOThread : public IOObject
{
friend class IOObject;
friend class IORunLoop;
friend class IOAutoreleasePool;
public:
	typedef void (*Function)(IOThread *thread);

	IOThread *initWithFunction(IOThread::Function function);

	void sleep(uint64_t time);
	void wakeup();
	void detach();

	void setName(IOString *name);
	void setPropertyForKey(IOObject *property, IOObject *key);

	IOString *name();
	IOObject *propertyForKey(IOObject *key); 

	IORunLoop *runLoop() const;
	IOAutoreleasePool *autoreleasePool() const;

	static IOThread *currentThread();
	static IOThread *withFunction(IOThread::Function function);

	static void yield();
	
private:
	virtual void free();

	void threadEntry();

	bool _detached;
	thread_t _id;
	Function _entry;

	kern_spinlock_t _lock;

	IOString *_name;
	IORunLoop *_runLoop;
	IOAutoreleasePool *_topPool;
	IODictionary *_properties;

	IODeclareClass(IOThread)
};

#endif /* _IOTHREAD_H_ */
