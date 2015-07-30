//
//  IOString.cpp
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2014 by Sidney Just
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
#include <libc/stdio.h>
#include <libcpp/functional.h>
#ifdef __KERNEL
#include <kern/kalloc.h>
#else
#include <libkern.h>
#endif
#include "IORuntime.h"
#include "IOString.h"

#define kIOStringFlagOwnedStorage (1 << 0)

namespace IO
{
	IODefineMeta(String, Object)

	String *String::Init()
	{
		if(!Object::Init())
			return nullptr;

		_length  = 0;
		_flags   = 0;
		_storage = const_cast<char *>("");

		CalculateHash();
		return this;
	}

	String *String::InitWithCString(const char *string, bool copy)
	{
		if(!Object::Init())
			return nullptr;

		_length = strlen(string);

		if(copy)
		{
			_flags = kIOStringFlagOwnedStorage;
			_storage = new char[_length + 1];
			strcpy(_storage, string);
		}
		else
		{
			_flags = 0;
			_storage = const_cast<char *>(string);
		}

		CalculateHash();
		return this;
	}

	String *String::InitWithFormat(const char *format, ...)
	{
		va_list args;
		va_start(args, format);

		String *result = InitWithFormatAndArguments(format, args);

		va_end(args);

		return result;
	}

	String *String::InitWithFormatAndArguments(const char *format, va_list args)
	{
		if(!Object::Init())
			return nullptr;

		_storage = new char[1024];
		_flags = kIOStringFlagOwnedStorage;

		vsnprintf(_storage, 1024, format, args);

		_length = strlen(_storage);

		CalculateHash();
		return this;
	}

	void String::Dealloc()
	{
		if(_flags & kIOStringFlagOwnedStorage)
			delete [] _storage;

		Object::Dealloc();
	}


	void String::CalculateHash()
	{
		_hash = 0;

		for(size_t i = 0; i < _length; i ++)
			std::hash_combine(_hash, _storage[i]);
	}

	size_t String::GetHash() const
	{
		return _hash;
	}

	bool String::IsEqual(Object *tother) const
	{
		String *other = tother->Downcast<String>();
		if(!other)
			return false;

		if(other->_length != _length)
			return false;

		return (strcmp(other->_storage, _storage) == 0);
	}

	bool String::IsEqual(const char *string) const
	{
		return (strcmp(string, _storage) == 0);
	}

	size_t String::GetLength() const
	{
		return _length;
	}

	char String::GetCharacterAtIndex(size_t index) const
	{
		IOAssert(index < _length, "GetCharacterAtIndex index out of bounds");
		return _storage[index];
	}

	const char *String::GetCString() const
	{
		return _storage;
	}
}
