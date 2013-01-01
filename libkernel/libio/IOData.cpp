//
//  IOData.cpp
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

#include <libkernel/kalloc.h>
#include <libkernel/string.h>
#include "IOData.h"

#ifdef super
#undef super
#endif
#define super IOObject

IORegisterClass(IOData, super)


IOData *IOData::init()
{
	if(!super::init())
		return 0;

	_buffer = 0;
	_length = 0;

	return this;
}

IOData *IOData::initWithBytes(uint8_t *bytes, size_t length)
{
	if(!super::init())
		return 0;

	_length = length;
	_buffer = (uint8_t *)kalloc(length);

	if(!_buffer)
	{
		release();
		return 0;
	}

	memcpy(_buffer, bytes, length);
	return this;
}

void IOData::free()
{
	if(_buffer)
		kfree(_buffer);

	super::free();
}


void IOData::appendBytes(uint8_t *bytes, size_t length)
{
	size_t nlength = _length + length;
	uint8_t *nbuffer = (uint8_t *)kalloc(nlength);

	if(nbuffer)
	{
		memcpy(nbuffer, _buffer, _length);
		memcpy(&nbuffer[_length], bytes, length);

		kfree(_buffer);

		_buffer = nbuffer;
		_length = length;
	}
}

uint8_t *IOData::bytes() const
{
	return _buffer;
}

size_t IOData::length() const
{
	return _length;
}
