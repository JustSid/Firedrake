//
//  IOModule.cpp
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

#include "IOModule.h"
#include "IOAutoreleasePool.h"

#ifdef super
#undef super
#endif
#define super IOProvider

IORegisterClass(IOModule, super);

static kern_spinlock_t __moduleLock = KERN_SPINLOCK_INIT;
static IODictionary *__moduleDictionary = 0;

IOModule *IOModule::initWithKmod(kern_module_t *kmod)
{
	if(super::init())
	{
		_published = false;
		_needsUnpublish = false;

		_publishLock = KERN_SPINLOCK_INIT_LOCKED;
		_kmod = kmod;
		_name = IOString::alloc()->initWithCString(kmod->name);

		kern_spinlock_lock(&__moduleLock);

		if(!__moduleDictionary)
			__moduleDictionary = IODictionary::alloc()->init();

		__moduleDictionary->setObjectForKey(this, _name);

		kern_spinlock_unlock(&__moduleLock);
		preparePublishing();
	}

	return this;
}

void IOModule::free()
{
	_name->release();
	super::free();
}



IOModule *IOModule::withName(const char *name)
{
	IOString *string = IOString::alloc()->initWithCString(name);
	IOModule *module = IOModule::withName(string);

	string->release();
	return module;
}

IOModule *IOModule::withName(IOString *name)
{
	IOModule *result = 0;
	kern_spinlock_lock(&__moduleLock);
	
	if(__moduleDictionary)
		result = (IOModule *)__moduleDictionary->objectForKey(name);

	kern_spinlock_unlock(&__moduleLock);

	if(!result)
	{
		kern_module_t *kmod = io_moduleWithName(name->CString());
		if(kmod)
			result = (IOModule *)kmod->module;
	}

	return result;
}


bool IOModule::isOnThread()
{
	return (_thread == IOThread::currentThread());
}

bool IOModule::isPublished()
{
	return _published;
}

void IOModule::waitForPublish()
{
	kern_spinlock_lock(&_publishLock);
	kern_spinlock_unlock(&_publishLock);
}



bool IOModule::publish()
{
	_runLoop = IORunLoop::currentRunLoop();
	return true;
}

void IOModule::unpublish()
{
	__moduleDictionary->removeObjectForKey(_name);

	_published = false;
	_kmod->module = 0;
}



void IOModule::finalizePublish(IOThread *thread)
{
	IOAutoreleasePool *pool = IOAutoreleasePool::alloc()->init();

	IOModule *module = (IOModule *)thread->propertyForKey(IOString::withCString("Owner"));
	kern_module_t *kmod = module->_kmod;

	module->_published = module->publish();
	pool->release();

	if(!module->_published)
	{
		kern_spinlock_lock(&__moduleLock);
		__moduleDictionary->removeObjectForKey(module->_name);
		kern_spinlock_unlock(&__moduleLock);

		module->_kmod = 0;
		module->release();

		io_moduleRelease(kmod);
		return;
	}

	pool = IOAutoreleasePool::alloc()->init();
	module->requestProbe();
	pool->release();

	kern_spinlock_unlock(&module->_publishLock);

	while(!module->_needsUnpublish)
		thread->runLoop()->run();

	pool = IOAutoreleasePool::alloc()->init();

	module->unpublish();
	module->release();
	
	pool->release();
	io_moduleRelease(kmod);
}

void IOModule::preparePublishing()
{
	IOString *key = IOString::alloc()->initWithCString("Owner");

	_thread = IOThread::alloc();
	_thread->initWithFunction(&IOModule::finalizePublish);
	_thread->setPropertyForKey(this, key);
	_thread->detach();
	_thread->release();

	key->release();
}

bool IOModule::requestUnpublish()
{
	if(_published)
	{
		_needsUnpublish = true;
		_thread->runLoop()->stop();

		return false;
	}
	
	return true;
}

