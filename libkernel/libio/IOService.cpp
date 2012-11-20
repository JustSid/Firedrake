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

#include <libkernel/interrupts.h>
#include "IOService.h"
#include "IORuntime.h"
#include "IODatabase.h"

#ifdef super
#undef super
#endif
#define super IOProvider

IORegisterClass(IOService, super);

IOService *IOService::init()
{
	if(super::init())
	{
		_provider = 0;
	}

	return this;
}

void IOService::free()
{
	super::free();
}



bool IOService::start(IOProvider *UNUSED(provider))
{
	return true;
}

void IOService::stop(IOProvider *UNUSED(provider))
{
	unpublishAllServices();
}




IOReturn IOService::registerInterrupt(uint32_t interrupt, InterruptAction handler, void *context)
{
	return kern_registerForInterrupt(interrupt, false, this, context, (kern_interrupt_handler_t)handler);
}

IOReturn IOService::unregisterInterrupt(uint32_t interrupt)
{
	return kern_registerForInterrupt(interrupt, false, this, 0, 0);
}



IOReturn IOService::registerService(IOSymbol *symbol, IODictionary *attributes)
{
	if(!symbol->inheritsFrom(IOSymbol::withName("IOService")))
	{
		panic("IOService::registerService(), symbol doesn't inherit from IOService!");
	}

	IODatabase *database = IODatabase::database();
	return database->registerService(symbol, attributes);
}

IOReturn IOService::unregisterService(IOSymbol *symbol)
{
	IODatabase *database = IODatabase::database();
	return database->unregisterService(symbol);
}

IODictionary *IOService::createMatchingDictionary(IOString *family, IODictionary *properties)
{
	IODictionary *dictionary = IODictionary::alloc()->init();

	dictionary->setObjectForKey(family, IOServiceAttributeFamily);

	if(properties != 0)
		dictionary->setObjectForKey(properties, IOServiceAttributeProperties);

	return dictionary->autorelease();
}
