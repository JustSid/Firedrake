//
//  kern_return.h
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

#ifndef _KERNRETURN_H_
#define _KERNRETURN_H_

#include <libc/stdint.h>
#include <libc/sys/errno.h>

#define KERN_SUCCESS 0
#define KERN_INVALID_ADDRESS    1
#define KERN_INVALID_ARGUMENT   2
#define KERN_NO_MEMORY          3
#define KERN_FAILURE            4
#define KERN_RESOURCES_MISSING  5
#define KERN_RESOURCE_IN_USE    6
#define KERN_RESOURCE_EXISTS    7
#define KERN_RESOURCE_EXHAUSTED 8

#if __KERNEL

#include <libcpp/type_traits.h>
#include "panic.h"

class Error
{
public:
	Error() :
		_code(KERN_SUCCESS)
	{}
	Error(uint32_t code) :
		_code(code)
	{}
	Error(uint32_t code, uint32_t errno) : 
		_code((errno << 24) | (code & 0xffffff))
	{}
	Error(const Error &other) :
		_code(other._code)
	{}

	Error &operator =(const Error &other)
	{
		_code = other._code;
		return *this;
	}

	bool operator ==(const Error &other) const { return (GetCode() == other.GetCode()); }
	bool operator !=(const Error &other) const { return (GetCode() != other.GetCode()); }

	operator bool () { return (GetCode() != KERN_SUCCESS); }

	uint32_t GetCode() const { return (_code & 0xffffff); }
	uint32_t GetErrno() const;

private:
	uint32_t _code;
};

#define ErrorNone Error(KERN_SUCCESS)

template<class T>
class KernReturn;

template<class T>
class KernReturn
{
public:
	KernReturn() :
		_error(KERN_SUCCESS),
		_acknowledged(false)
	{}
	
	KernReturn(const T &value) :
		_value(value),
		_error(KERN_SUCCESS),
		_acknowledged(false)
	{}
	
	KernReturn(const Error &e) :
		_error(e),
		_acknowledged(false)
	{}
	
	KernReturn(KernReturn &&other) :
		_error(other._error),
		_value(std::move(other._value)),
		_acknowledged(other._acknowledged)
	{
		other._acknowledged = true;
	}
	
	KernReturn &operator =(KernReturn &&other)
	{
		_error        = other._error;
		_value        = std::move(other._value);
		_acknowledged = other._acknowledged;
		
		other._acknowledged = true;
		return *this;
	}
	
	~KernReturn()
	{
		if(!_acknowledged)
			panic("Unacknowledged KernReturn, code: %d!", _error.GetCode());
	}
	
	operator T() const { return Get(); }
	operator Error() const { _acknowledged = true; return _error; }

	bool IsValid() const { _acknowledged = true; return (_error.GetCode() == KERN_SUCCESS); }
	Error GetError() const { return _error; }
	T Get() const { Validate(); return _value; }

	void Suppress() { _acknowledged = true; }
	
private:
	void Validate() const { if(!IsValid()) { panic("Attempted to access an invalid KernReturn"); } }

	T _value;
	Error _error;
	mutable bool _acknowledged;
};

template<class T>
class KernReturn<T *>
{
public:
	KernReturn() :
		_error(KERN_SUCCESS),
		_acknowledged(false),
		_value(nullptr)
	{}
	
	KernReturn(T *value) :
		_value(value),
		_error(KERN_SUCCESS),
		_acknowledged(false)
	{}
	
	KernReturn(const Error &e) :
		_error(e),
		_acknowledged(false)
	{}
	
	KernReturn(KernReturn &&other) :
		_error(other._error),
		_value(std::move(other._value)),
		_acknowledged(other._acknowledged)
	{
		other._acknowledged = true;
	}
	
	KernReturn &operator =(KernReturn &&other)
	{
		_error        = other._error;
		_value        = std::move(other._value);
		_acknowledged = other._acknowledged;
		
		other._acknowledged = true;
		return *this;
	}
	
	~KernReturn()
	{
		if(!_acknowledged)
			panic("Unacknowledged KernReturn, code: %d!", _error.GetCode());
	}
	
	operator T *() const { return Get(); }
	operator Error() const { _acknowledged = true; return _error; }

	T *operator ->() { return Get(); }
	const T *operator ->() const { return Get(); }
	
	bool IsValid() const { _acknowledged = true; return (_error.GetCode() == KERN_SUCCESS); }
	Error GetError() const { return _error; }
	T *Get() const { Validate(); return _value; }

	void Suppress() { _acknowledged = true; }
	
private:
	void Validate() const { if(!IsValid()) { panic("Attempted to access an invalid KernReturn"); } }

	T *_value;
	Error _error;
	mutable bool _acknowledged;
};

template<>
struct KernReturn<void>
{
public:
	KernReturn() :
		_error(KERN_SUCCESS),
		_acknowledged(false)
	{}
	
	KernReturn(const Error &e) :
		_error(e),
		_acknowledged(false)
	{}
	
	KernReturn(KernReturn &&other) :
		_error(other._error),
		_acknowledged(other._acknowledged)
	{
		other._acknowledged = true;
	}
	
	KernReturn &operator =(KernReturn &&other)
	{
		_error = other._error;
		_acknowledged = other._acknowledged;
		
		other._acknowledged = true;
		return *this;
	}
	
	~KernReturn()
	{
		if(!_acknowledged && _error.GetCode() != KERN_SUCCESS)
			panic("Unacknowledged KernReturn, code: %d!", _error.GetCode());
	}

	operator Error() const { _acknowledged = true; return _error; }
	
	bool IsValid() const { _acknowledged = true; return (_error.GetCode() == KERN_SUCCESS); }
	Error GetError() const { return _error; }

	void Suppress() { _acknowledged = true; }
	
private:
	Error _error;
	mutable bool _acknowledged;
};

#endif /* __KERNEL */
#endif /* _KERNRETURN_H_ */
