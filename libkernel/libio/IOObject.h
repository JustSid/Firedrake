//
//  IOObject.h
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

#ifndef _IOOBJECT_H_
#define _IOOBJECT_H_

#include <libkernel/libkernel.h>
#include "IOTypes.h"
#include "IOSymbol.h"
#include "IORuntime.h"

class IOString;
class IODatabase;

class IOObject
{
friend class IOSymbol;
friend class IODatabase;
public:
	void release();
	virtual IOObject *retain();
	virtual IOObject *autorelease();

	static IOObject *alloc();
	IOSymbol *symbol() const;

	virtual hash_t hash() const;
	virtual bool isEqual(const IOObject *other) const;

	bool isKindOf(IOObject *other) const;
	bool isSubclassOf(IOSymbol *symbol) const;
	bool isSubclassOf(const char *name) const;

	virtual IOString *description() const;

protected:
	virtual IOObject *init();
	virtual void free();

	void prepareWithSymbol(IOSymbol *symbol);

private:
	IOSymbol *_symbol;
	uint32_t _retainCount;
	kern_spinlock_t _lock;
};

#define __IOTokenExpand(tok) #tok
#define __IOToken(tok) __IOTokenExpand(tok)

#define IODeclareClass(className) \
	public: \
		virtual className *retain(); \
		virtual className *autorelease(); \
		static className *alloc(); \
	private: \

#define IORegisterClass(className, super) \
	static IOSymbol *__##className##__symbol = 0; \
	className *className::alloc() \
	{ \
		IOSymbol *symbol = __##className##__symbol ; \
		className *instance = new className; \
		instance->prepareWithSymbol(symbol); \
		return instance; \
	} \
	className *className::retain() \
	{ \
		return (className *)super::retain(); \
	} \
	className *className::autorelease() \
	{ \
		return (className *)super::autorelease(); \
	} \
	void __##className##__load() __attribute((constructor)); \
	void __##className##__load() \
	{ \
		__##className##__symbol = new IOSymbol(#className, __IOToken(super), sizeof(className), (IOSymbol::AllocCallback)&className::alloc); \
	}

#endif /* _IOOBJECT_H_ */
