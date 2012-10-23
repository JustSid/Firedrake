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

#include <libc/string.h>
#include "IOMemory.h"
#include "IOModule.h"
#include "IODatabase.h"
#include "IOLog.h"
/*
#ifdef super
#undef super
#endif
#define super IOObject

bool IOModule::init()
{
	if(!super::init())
		return false;
	
	services = IOArray::withCapacity(1);
	identifier = 0;
	
	return (services != 0);
}

bool IOModule::initWithIdentifier(const char *tidentifier)
{
	if(!init())
		return false;
	
	size_t length = strlen(tidentifier) + 1;
	identifier = (const char *)IOMalloc(length * sizeof(char));
	
	if(!identifier)
		return false;
	
	strlcpy((char *)identifier, tidentifier, length);
	return true;
}



bool IOModule::publishService(IOService *service)
{
	if(service->isRunning())
		return false;
	
	bool result = service->start();
	if(result)
	{
		services->addObject(service);
		return true;
	}

	return false;
}

bool IOModule::unpublishService(IOService *service)
{
	uint32_t index = services->indexOfObject(service);
	if(index != IONotFound)
	{
		if(service->willStop())
		{
			service->stop();
			services->removeObject(index);
			
			return true;
		}
	}
	
	return false;
}



bool IOModule::publish()
{
	this->retain();
	return true;
}

bool IOModule::willUnpublish()
{
	return true;
}


bool IOModule::unpublish()
{
	if(!willUnpublish())
		return false;
	
	// Try to unpublish all services
	bool unpublishedServices = true;
	for(uint32_t i=0; i<services->getCount(); i++)
	{
		IOService *service = (IOService *)services->objectAtIndex(i);
		if(service->willStop())
		{
			service->stop();
			services->removeObject(i);
			
			i --;
		}
		else
		{
			unpublishedServices = false;
		}
	}
	
	if(unpublishedServices)
	{
		this->release();
		return true;
	}
	
	return false;
}*/

