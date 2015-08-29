//
//  IOCatalogue.cpp
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

#include "IOCatalogue.h"
#include "IOArray.h"
#include "IODictionary.h"
#include "IOSet.h"
#include "IOString.h"
#include "IONumber.h"
#include "IONull.h"

#ifndef __KERNEL
extern "C" void *__libio_getIOCatalogue();
#include <libkern.h>
#else
#include <kern/kprintf.h>
#endif

namespace IO
{
	// -------------
	// MARK: -
	// MARK: MetaClass
	// -------------

	MetaClass::MetaClass(MetaClass *super, const char *signature) :
		_super(super)
	{
		size_t length = strlen(signature);

		_fullname = new char[length];
		_name     = new char[length];

		// Find the end
		// IO::ArrayMetaType::ArrayMetaType()
		//          ^-- This part
		const char *end = signature;
		while(1)
		{
			const char *temp = strstr((char *)end, "::");
			if(!temp)
				break;
			
			end = temp + 2;
		}

		end -= 2;
		end -= 8; // strlen("MetaType");

		// Find the end
		// IO::ArrayMetaType
		//     ^-- This part
		const char *begin = signature;
		while(1)
		{
			const char *temp = strstr((char *)begin, "::");
			if(!temp || temp >= end)
				break;
			
			begin = temp + 2;
		}

		strlcpy(_name, begin, end - begin);
		strlcpy(_fullname, signature, end - signature);

		Catalogue::GetSharedInstance()->AddMetaClass(this);
	}

	MetaClass *MetaClass::GetSuperClass() const
	{
		return _super;
	}

	const char *MetaClass::GetName() const
	{
		return _name;
	}

	const char *MetaClass::GetFullname() const
	{
		return _fullname;
	}

	bool MetaClass::InheritsFromClass(MetaClass *other) const
	{
		if(this == other)
			return true;

		if(!_super)
			return false;

		return _super->InheritsFromClass(other);
	}

	// -------------
	// MARK: -
	// MARK: Catalogue
	// -------------

#if __KERNEL
	static Catalogue *_sharedCatalogue = nullptr;
	Catalogue::Catalogue()
	{
		spinlock_init(&_lock);
	}

	Catalogue *Catalogue::GetSharedInstance()
	{
		return _sharedCatalogue;
	}

	struct __CatalogueHelper
	{
		static Catalogue *Construct() { return new Catalogue(); }
	};
#else
	Catalogue::Catalogue()
	{}
	Catalogue *Catalogue::GetSharedInstance()
	{
		return reinterpret_cast<Catalogue *>(__libio_getIOCatalogue());
	}
#endif

	MetaClass *Catalogue::GetClassWithName(const char *name)
	{
		spinlock_lock(&_lock);

		MetaClass *result = nullptr;
		
		size_t size = _classes.size();
		for(size_t i = 0; i < size; i ++)
		{
			MetaClass *meta = _classes.at(i);

			if(strcmp(meta->GetFullname(), name) == 0)
			{
				result = meta;
				break;
			}
		}

		spinlock_unlock(&_lock);

		return result;
	}

	void Catalogue::AddMetaClass(MetaClass *meta)
	{
		spinlock_lock(&_lock);
		_classes.push_back(meta);
		spinlock_unlock(&_lock);
	}

#if __KERNEL
	// -------------
	// MARK: -
	// MARK: Catalogue Registration
	// -------------

	static Catalogue::__MetaClassGetter __metaClassGetter[128];
	static size_t __metaClassGetterCount = 0;

	void Catalogue::__MarkClass(__MetaClassGetter entry)
	{
		__metaClassGetter[__metaClassGetterCount ++] = entry;
	}

	KernReturn<void> CatalogueInit()
	{
		_sharedCatalogue = __CatalogueHelper::Construct();

		// Run all GetMetaClass() for kernel classes
		Object::GetMetaClass();

		for(size_t i = 0; i < __metaClassGetterCount; i ++)
		{
			__metaClassGetter[i]();
		}


		Null::GetNull(); // Get the shared Null class to avoid race conditions

		return ErrorNone;
	}
#endif
}
