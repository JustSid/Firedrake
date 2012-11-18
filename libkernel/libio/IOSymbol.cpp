//
//  IOSymbol.cpp
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

#include "IOSymbol.h"
#include "IOObject.h"
#include "IOString.h"
#include "IORuntime.h"
#include "IODatabase.h"
#include "IOLog.h"

// Constructor

IOSymbol::IOSymbol(const char *name, const char *super, size_t size)
{
	_size = size;

	_name = new IOString;
	_name->prepareWithSymbol(0);
	_name->initWithCString(name);

	_superName = super;
	_super = 0;

	bool result = IODatabase::database()->publishSymbol(this);
	if(!result)
		panic("Couldn't publish symbol %s!", name);
}



IOSymbol *IOSymbol::withName(const char *name)
{
	return IODatabase::database()->symbolWithName(name);
}

IOSymbol *IOSymbol::withName(IOString *name)
{
	return IODatabase::database()->symbolWithName(name);
}



bool IOSymbol::inheritsFrom(IOSymbol *other)
{
	if(this == other)
		return true;

	if(super() == 0)
		return false;

	return super()->inheritsFrom(other);
}


const IOString *IOSymbol::name() const
{
	return _name;
}

size_t IOSymbol::size() const
{
	return _size;
}

IOObject *IOSymbol::alloc()
{
	IOObject *object = (IOObject *)kalloc(_size);
	object->prepareWithSymbol(this);

	return object;
}

IOSymbol *IOSymbol::super()
{
	if(!_superName)
		return 0;

	if(!_super)
		_super = IOSymbol::withName(_superName);

	return _super;
}
