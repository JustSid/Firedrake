//
//  IOService.cpp
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

#include "IOService.h"
#include "IORuntime.h"
#include "IODatabase.h"

#ifdef super
#undef super
#endif
#define super IOObject

IORegisterClass(IOService, super);

extern "C" IOReturn __IORegisterForInterrupt(uint32_t interrupt, bool exclusive, void *owner, void *target, void *context, IOService::InterruptAction callback);

IOService *IOService::init()
{
	if(super::init())
	{
		_published = IOArray::alloc()->init();
		if(!_published)
		{
			release();
			return 0;
		}

		_provider = 0;
	}

	return this;
}

void IOService::free()
{
	if(_published)
	{
		for(size_t i=0; i<_published->count(); i++)
		{
			IOService *service = (IOService *)_published->objectAtIndex(i);
			service->stop();
			service->_provider = 0;

			service->release();
		}
	}

	super::free();
}



bool IOService::start()
{
	return true;
}

void IOService::stop()
{
	for(size_t i=0; i<_published->count();)
	{
		IOService *service = (IOService *)_published->objectAtIndex(i);
		service->stop();
		service->setProvider(0);

		_published->removeObject(i);
	}
}



void IOService::requestProbe()
{
}



IOReturn IOService::registerInterrupt(uint32_t interrupt, IOObject *target, InterruptAction handler, void *context)
{
	return __IORegisterForInterrupt(interrupt, false, this, target, context, handler);
}

IOReturn IOService::unregisterInterrupt(uint32_t interrupt)
{
	return __IORegisterForInterrupt(interrupt, false, this, 0, 0, 0);
}



IOReturn IOService::registerService(IOSymbol *symbol, IODictionary *attributes)
{
	if(!symbol->inheritsFrom(IOSymbol::withName("IOService")))
	{
		panic("IOService::registerService(), symbol doesn't inherit from IOService!");
	}

	if(!attributes->objectForKey(IOServiceAttributeIdentifier))
	{
		panic("IOService::registerService(), attributes without identifer!");
	}

	IODatabase *database = IODatabase::database();
	return database->registerService(symbol, attributes);
}

IOReturn IOService::unregisterService(IOSymbol *symbol)
{
	IODatabase *database = IODatabase::database();
	return database->unregisterService(symbol);
}


void IOService::setProvider(IOService *provider)
{
	_provider = provider;
}

IOService *IOService::findMatchingService(IODictionary *attributes)
{
	IODatabase *database = IODatabase::database();
	return database->serviceMatchingAttributes(attributes);
}

void IOService::publishService(IOService *service)
{
	bool result = service->start();
	if(result)
	{
		service->setProvider(this);
		_published->addObject(service);
	}
}

void IOService::unpublishService(IOService *service)
{
	if(_published->containsObject(service))
	{
		service->stop();
		service->setProvider(0);
		
		_published->removeObject(service);
	}
}
