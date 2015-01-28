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

#include <kern/kprintf.h>
#include "IOCatalogue.h"
#include "IOArray.h"
#include "IODictionary.h"
#include "IOSet.h"
#include "IOString.h"
#include "IONumber.h"
#include "IONull.h"

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

	static Catalogue *_sharedCatalogue = nullptr;

	Catalogue::Catalogue() :
		_lock(SPINLOCK_INIT)
	{}

	Catalogue *Catalogue::GetSharedInstance()
	{
		return _sharedCatalogue;
	}

	MetaClass *Catalogue::GetClassWithName(const char *name)
	{
		spinlock_lock(&_lock);

		MetaClass *result = nullptr;
		
		size_t size = _classes.size();
		for(size_t i = 0; i < size; i ++)
		{
			MetaClass *meta = _classes.at(i);

			if(strcmp(meta->GetName(), name) == 0)
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

	KernReturn<void> CatalogueInit()
	{
		Object::GetMetaClass();
		Array::GetMetaClass();
		Dictionary::GetMetaClass();
		Set::GetMetaClass();
		String::GetMetaClass();
		Number::GetMetaClass();
		Null::GetMetaClass();

		Null::GetNull(); // Get the shared Null class to avoid race conditions

		return ErrorNone;
	}
#endif
}
