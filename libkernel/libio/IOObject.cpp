//
//  IOObject.cpp
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

#include <libkernel/kalloc.h>
#include "IOObject.h"
#include "IOString.h"
#include "IOSymbol.h"
#include "IOThread.h"
#include "IOAutoreleasePool.h"
#include "IORuntime.h"
#include "IOLog.h"

IOObject *IOObject::init()
{
	return this;
}

void IOObject::free()
{
}

void IOObject::prepareWithSymbol(IOSymbol *symbol)
{
	_symbol = symbol;
	retainCount = 1;
}




void IOObject::release()
{
	if((-- retainCount) == 0)
	{
		free();
		kfree(this);

		return;
	}
}

IOObject *IOObject::retain()
{
	retainCount ++;
	return this;
}

IOObject *IOObject::autorelease()
{
	if(!this)
		return this;

	IOThread *thread = IOThread::currentThread();
	if(!thread->_topPool)
	{
		IOLog("%s::autorelease() with no pool in place, leaking.", _symbol->name()->CString());
		return this;
	}

	thread->_topPool->addObject(this);
	return this;
}



IOSymbol *IOObject::symbol() const
{
	return _symbol;
}

IOString *IOObject::description() const
{
	IOString *string = IOString::withFormat("<%s:%p>", _symbol->name()->CString(), this);
	return string;
}



hash_t IOObject::hash() const
{
	return (hash_t)this;
}

bool IOObject::isEqual(const IOObject *other) const
{
	return (this == other);
}



bool IOObject::isKindOf(IOObject *other) const
{
	IOSymbol *otherSymbol = other->symbol();
	return _symbol->inheritsFrom(otherSymbol);
}

bool IOObject::isSubclassOf(IOSymbol *symbol) const
{	
	if(_symbol == symbol)
		return true;

	if(!_symbol)
		return false;

	return _symbol->inheritsFrom(symbol);
}

bool IOObject::isSubclassOf(const char *name) const
{
	IOSymbol *otherSymbol = IOSymbol::withName(name);
	if(!otherSymbol)
		return (_symbol == 0);

	return _symbol->inheritsFrom(otherSymbol);
}



static IOSymbol *__IOObjectSymbol = 0;

IOObject *IOObject::alloc()
{
	panic("IOObject::alloc() called directly!");
	return 0;
}

void __IOObject__load() __attribute__((constructor));
void __IOObject__load()
{
	__IOObjectSymbol = new IOSymbol("IOObject", 0, sizeof(IOObject));
}
