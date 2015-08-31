//
//  IORuntime.cpp
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

#include "IORuntime.h"

#if __KERNEL
#error "Don't include IORuntime.cpp into the kernel"
#endif

#include "IOObject.h"
#include "../service/IOService.h"
#include "../service/IOResources.h"

namespace IO
{
	class ClassEntry : public Object
	{
	public:
		ClassEntry *InitWithClass(MetaClass *meta, Dictionary *properties)
		{
			if(!Object::Init())
				return nullptr;

			_meta = meta;
			_properties = properties->Retain();
			_matched = false;

			return this;
		}

		void SetMatched(bool matched)
		{
			_matched = true;
		}

		Dictionary *GetProperties() const { return _properties; }
		MetaClass *GetMeta() const { return _meta; }
		bool IsMatched() const { return _matched; }

	private:
		MetaClass *_meta;
		Dictionary *_properties;
		bool _matched;

		IODeclareMeta(ClassEntry)
	};


	IODefineMeta(ClassEntry, Object)

	static spinlock_t _matchingLock;
	static Array *_registeredClasses;
	static Array *_providers;


	bool DoShallowMatch(MetaClass *providerClass, ClassEntry *entry)
	{
		String *requiredProvider = entry->GetProperties()->GetObjectForKey<String>(kServiceProviderMatchKey);
		if(!requiredProvider)
			return false;

		MetaClass *meta = Catalogue::GetSharedInstance()->GetClassWithName(requiredProvider->GetCString());
		if(!meta)
			return false;

		return (providerClass->InheritsFromClass(meta));
	}

	void RegisterClass(MetaClass *meta, Dictionary *properties)
	{
		ClassEntry *entry = ClassEntry::Alloc()->InitWithClass(meta, properties);

		spinlock_lock(&_matchingLock);
		_registeredClasses->AddObject(entry);

		Array *providersCopy = Array::Alloc()->InitWithArray(_providers);
		spinlock_unlock(&_matchingLock);


		// Ping all provider to allow them to match against the newly added class
		for(size_t i = 0; i < providersCopy->GetCount(); i ++)
		{
			Service *provider = providersCopy->GetObjectAtIndex<Service>(i);
			provider->StartMatching();
		}

		providersCopy->Release();
	}

	void RegisterProvider(Service *provider)
	{
		spinlock_lock(&_matchingLock);
		_providers->AddObject(provider);
		spinlock_unlock(&_matchingLock);

		provider->StartMatching();
	}

	void EnumerateClassesForProvider(Service *provider, const Function<bool (MetaClass *meta, Dictionary *properties)> &callback)
	{
		MetaClass *providerClass = provider->GetClass();

		spinlock_lock(&_matchingLock);

		Array *matches = Array::Alloc()->Init();

		for(size_t i = 0; i < _registeredClasses->GetCount(); i ++)
		{
			ClassEntry *entry = _registeredClasses->GetObjectAtIndex<ClassEntry>(i);

			if(!entry->IsMatched() && DoShallowMatch(providerClass, entry))
				matches->AddObject(entry);
		}

		spinlock_unlock(&_matchingLock);

		for(size_t i = 0; i < matches->GetCount(); i ++)
		{
			ClassEntry *entry = matches->GetObjectAtIndex<ClassEntry>(i);

			if(callback(entry->GetMeta(), entry->GetProperties()))
				entry->SetMatched(true);
		}

		matches->Release();
	}
}




extern "C" bool IORuntimeBootstrap();

bool IORuntimeBootstrap()
{
	spinlock_init(&IO::_matchingLock);
	IO::_registeredClasses = IO::Array::Alloc()->Init();
	IO::_providers = IO::Array::Alloc()->Init();


	IO::Resources *resources = IO::Resources::Alloc()->Init();
	IO::RegistryEntry *rootEntry = IO::RegistryEntry::GetRootEntry();

	rootEntry->AttachChild(resources);

	return (IO::_registeredClasses && IO::_providers);
}
