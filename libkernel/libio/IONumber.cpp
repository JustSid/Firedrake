//
//  IONumber.cpp
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

#include <libkernel/string.h>
#include <libkernel/kalloc.h>

#include "IONumber.h"
#include "IOString.h"
#include "IOAutoreleasePool.h"

#ifdef super
#undef super
#endif
#define super IOObject

IORegisterClass(IONumber, super);

#define IONumberSimpleAccess(number, type) (*((type *)number->_buffer))

IONumber *IONumber::initWithBuffer(IONumberType type, void *blob, size_t size)
{
	if(super::init())
	{
		_buffer = kalloc(size);
		if(!_buffer)
		{
			release();
			return 0;
		}

		memcpy(_buffer, blob, size);
		_type = type;
	}

	return this;
}

IONumber *IONumber::initWithInt8(int8_t val)
{
	return initWithBuffer(kIONumberTypeInt8, &val, sizeof(int8_t));
}

IONumber *IONumber::initWithInt16(int16_t val)
{
	return initWithBuffer(kIONumberTypeInt16, &val, sizeof(int16_t));
}

IONumber *IONumber::initWithInt32(int32_t val)
{
	return initWithBuffer(kIONumberTypeInt32, &val, sizeof(int32_t));
}

IONumber *IONumber::initWithInt64(int64_t val)
{
	return initWithBuffer(kIONumberTypeInt64, &val, sizeof(int64_t));
}

IONumber *IONumber::initWithUInt8(uint8_t val)
{
	return initWithBuffer(kIONumberTypeUInt8, &val, sizeof(uint8_t));
}

IONumber *IONumber::initWithUInt16(uint16_t val)
{
	return initWithBuffer(kIONumberTypeUInt16, &val, sizeof(uint16_t));
}

IONumber *IONumber::initWithUInt32(uint32_t val)
{
	return initWithBuffer(kIONumberTypeUInt32, &val, sizeof(uint32_t));
}

IONumber *IONumber::initWithUInt64(uint64_t val)
{
	return initWithBuffer(kIONumberTypeUInt64, &val, sizeof(uint64_t));
}

void IONumber::free()
{
	if(_buffer)
		kfree(_buffer);

	super::free();
}

IONumber *IONumber::withInt8(int8_t val)
{
	return IONumber::alloc()->initWithInt8(val)->autorelease();
}
IONumber *IONumber::withInt16(int16_t val)
{
	return IONumber::alloc()->initWithInt16(val)->autorelease();
}
IONumber *IONumber::withInt32(int32_t val)
{
	return IONumber::alloc()->initWithInt32(val)->autorelease();
}
IONumber *IONumber::withInt64(int64_t val)
{
	return IONumber::alloc()->initWithInt64(val)->autorelease();
}
IONumber *IONumber::withUInt8(uint8_t val)
{
	return IONumber::alloc()->initWithUInt8(val)->autorelease();
}
IONumber *IONumber::withUInt16(uint16_t val)
{
	return IONumber::alloc()->initWithUInt16(val)->autorelease();
}
IONumber *IONumber::withUInt32(uint32_t val)
{
	return IONumber::alloc()->initWithUInt32(val)->autorelease();
}
IONumber *IONumber::withUInt64(uint64_t val)
{
	return IONumber::alloc()->initWithUInt64(val)->autorelease();
}


#define IONumberAccessAndConvert(type) \
	do { \
		switch(_type) \
		{ \
			case kIONumberTypeInt8: \
			case kIONumberTypeUInt8: \
				return (type)IONumberSimpleAccess(this, int8_t); \
				break; \
			case kIONumberTypeInt16: \
			case kIONumberTypeUInt16: \
				return (type)IONumberSimpleAccess(this, int16_t); \
				break; \
			case kIONumberTypeInt32: \
			case kIONumberTypeUInt32: \
				return (type)IONumberSimpleAccess(this, int32_t); \
				break; \
			case kIONumberTypeInt64: \
			case kIONumberTypeUInt64: \
				return (type)IONumberSimpleAccess(this, int64_t); \
				break; \
		} \
	} \
	while(0)


int8_t IONumber::int8Value() const
{
	IONumberAccessAndConvert(int8_t);
}

int16_t IONumber::int16Value() const
{
	IONumberAccessAndConvert(int16_t);
}

int32_t IONumber::int32Value() const
{
	IONumberAccessAndConvert(int32_t);
}

int64_t IONumber::int64Value() const
{
	IONumberAccessAndConvert(int64_t);
}

uint8_t IONumber::uint8Value() const
{
	IONumberAccessAndConvert(uint8_t);
}

uint16_t IONumber::uint16Value() const
{
	IONumberAccessAndConvert(uint16_t);
}

uint32_t IONumber::uint32Value() const
{
	IONumberAccessAndConvert(uint32_t);
}

uint64_t IONumber::uint64Value() const
{
	IONumberAccessAndConvert(uint64_t);
}


hash_t IONumber::hash() const
{
	hash_t hash = (hash_t)uint32Value();
	return hash;
}

bool IONumber::isEqual(const IOObject *other) const
{
	if(!other->isSubclassOf(symbol()))
		return false;

	IONumber *number = (IONumber *)other;
	if(_type == number->_type)
	{
		switch(_type)
		{
			case kIONumberTypeInt8:
			case kIONumberTypeUInt8:
				return (IONumberSimpleAccess(this, int8_t) == IONumberSimpleAccess(number, int8_t));
				break;

			case kIONumberTypeInt16:
			case kIONumberTypeUInt16:
				return (IONumberSimpleAccess(this, int16_t) == IONumberSimpleAccess(number, int16_t));
				break;

			case kIONumberTypeInt32:
			case kIONumberTypeUInt32:
				return (IONumberSimpleAccess(this, int32_t) == IONumberSimpleAccess(number, int32_t));
				break;

			case kIONumberTypeInt64:
			case kIONumberTypeUInt64:
				return (IONumberSimpleAccess(this, int64_t) == IONumberSimpleAccess(number, int64_t));
				break;
		}
	}

	return false;
}

IOString *IONumber::description() const
{
	switch(_type)
	{
		case kIONumberTypeInt8:
		case kIONumberTypeInt16:
		case kIONumberTypeInt32:
		case kIONumberTypeInt64:
			return IOString::withFormat("%i", int32Value());
			break;

		case kIONumberTypeUInt8:
		case kIONumberTypeUInt16:
		case kIONumberTypeUInt32:
		case kIONumberTypeUInt64:
			return IOString::withFormat("%u", uint32Value());
			break;
	}
}
