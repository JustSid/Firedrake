//
//  IOModule.cpp
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

#include "IOModule.h"
#include "IOAutoreleasePool.h"

#ifdef super
#undef super
#endif
#define super IOObject

IORegisterClass(IOModule, super);

IOModule *IOModule::initWithKmod(kern_module_t *kmod)
{
	if(super::init())
	{
		_providers = IOArray::alloc()->init();
		if(!_providers)
		{
			release();
			return 0;
		}

		_published = false;
		_kmod = kmod;

		preparePublishing();
	}

	return this;
}

void IOModule::free()
{
	_providers->release();
	super::free();
}


bool IOModule::publish()
{
	return true;
}

void IOModule::unpublish()
{
	if(_published && !IOThread::currentThread()->isEqual(_thread))
	{
		_thread->getRunLoop()->stop();
		return;
	}

	for(size_t i=0; i<_providers->count();)
	{
		IOService *service = (IOService *)_providers->objectAtIndex(i);
		service->stop();

		_providers->removeObject(i);
	}
}


void IOModule::finalizePublish(IOThread *thread)
{
	IOAutoreleasePool *pool = IOAutoreleasePool::alloc()->init();

	IOModule *module = (IOModule *)thread->propertyForKey(IOString::withCString("Owner"));
	module->_published = module->publish();

	pool->release();

	if(module->_published)
		thread->getRunLoop()->run();

	kern_moduleRelease(module->_kmod);
}

void IOModule::preparePublishing()
{
	IOString *key = IOString::alloc();
	key->initWithCString("Owner");

	_thread = IOThread::alloc();
	_thread->initWithFunction(&IOModule::finalizePublish);
	_thread->setPropertyForKey(this, key);
	_thread->detach();

	key->release();
}
