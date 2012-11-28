//
//  IOProvider.cpp
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

#include "IOProvider.h"
#include "IODatabase.h"
#include "IOService.h"

#ifdef super
#undef super
#endif
#define super IOObject

IORegisterClass(IOProvider, super);

IOProvider *IOProvider::init()
{
	if(!super::init())
		return 0;

	_services = IOSet::alloc()->init();
	_runLoop = IORunLoop::currentRunLoop();

	return this;
}

void IOProvider::free()
{
	if(_services)
	{	
		unpublishAllServices();
		_services->release();
	}

	super::free();
}



bool IOProvider::publishService(IOService *service)
{
	service->_provider = this;
	service->_runLoop = _runLoop;

	if(service->start(this))
	{
		_services->addObject(service);
		service->requestProbe();

		return true;
	}
	
	service->_provider = 0;
	return false;
}

void IOProvider::unpublishService(IOService *service)
{
	if(_services->containsObject(service))
	{
		service->stop(this);
		service->_provider = 0;

		_services->removeObject(service);
	}
}

void IOProvider::unpublishAllServices()
{
	IOIterator *iterator = _services->objectIterator();
	IOService *service;

	while((service = (IOService *)iterator->nextObject()))
	{
		service->stop(this);
		service->_provider = 0;
	}

	_services->removeAllObjects();
}


IOService *IOProvider::findMatchingService(IODictionary *attributes)
{
	IODatabase *database = IODatabase::database();
	return database->serviceMatchingAttributes(attributes);
}
