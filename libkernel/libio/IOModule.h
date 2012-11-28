//
//  IOModule.h
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

#ifndef _IOMODULE_H_
#define _IOMODULE_H_

#include <libkernel/entry.h>
#include <libkernel/module.h>

#include "IOObject.h"
#include "IOProvider.h"
#include "IOArray.h"
#include "IOService.h"
#include "IOThread.h"

class IOModule : public IOProvider
{
public:
	virtual IOModule *initWithKmod(kern_module_t *kmod);
	virtual void free();

	static IOModule *withName(const char *name);
	static IOModule *withName(IOString *name);

	virtual bool publish();
	virtual void unpublish();

	bool requestUnpublish();
	bool isPublished();

	bool isOnThread();
	void waitForPublish();

	IOThread *getThread() { return _thread; }
	kern_module_t *getKernelModule() { return _kmod; }

private:
	static void finalizePublish(IOThread *thread);
	void preparePublishing();

	IOThread *_thread;
	IOString *_name;

	bool _published;
	bool _needsUnpublish;

	kern_spinlock_t _publishLock;
	kern_module_t *_kmod;

	IODeclareClass(IOModule)
};

#define IORegisterModule(class) \
	extern "C" { \
		bool _kern_start(kern_module_t *kmod) \
		{ \
			IOModule *module = class::alloc()->initWithKmod(kmod); \
			kmod->module = (void *)module; \
			return (module != 0); \
		} \
		bool _kern_stop(kern_module_t *kmod) \
		{ \
			IOModule *module = (IOModule *)kmod->module; \
			if(module) \
			{ \
				module->requestUnpublish(); \
				return false; \
			} \
			return true; \
		} \
	} \
	
#endif /* _IOMODULE_H_ */
