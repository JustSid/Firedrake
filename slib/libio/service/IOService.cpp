//
//  IOService.cpp
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2015 by Sidney Just
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

#include "IOService.h"

namespace IO
{
	IODefineMeta(Service, RegistryEntry)

	String *kServiceProviderMatchKey;
	String *kServiceClassMatchKey;
	String *kServicePropertiesMatchKey;

	extern void RegisterClass(MetaClass *meta, Dictionary *properties);
	extern void RegisterProvider(Service *provider);
	extern void EnumerateClassesForProvider(Service *provider, const Function<bool (MetaClass *meta, Dictionary *properties)> &callback);

	void Service::InitialWakeUp(MetaClass *meta)
	{
		if(meta == Service::GetMetaClass())
		{
			kServiceProviderMatchKey = String::Alloc()->InitWithCString("kServiceProviderMatchKey");
			kServiceClassMatchKey = String::Alloc()->InitWithCString("kServiceClassMatchKey");
			kServicePropertiesMatchKey = String::Alloc()->InitWithCString("kServicePropertiesMatchKey");
		}

		RegistryEntry::InitialWakeUp(meta);
	}

	void Service::RegisterService(MetaClass *meta, Dictionary *properties)
	{
		RegisterClass(meta, properties);
	}


	Service *Service::InitWithProperties(Dictionary *properties)
	{
		if(!RegistryEntry::Init())
			return nullptr;

		_started = false;
		_properties = properties->Retain();

		return this;
	}

	Dictionary *Service::GetProperties() const
	{
		return _properties;
	}

	void Service::Start()
	{
		_started = true;
	}

	void Service::Stop()
	{}


	// Matching
	void Service::RegisterProvider()
	{
		IO::RegisterProvider(this);
	}

	bool Service::MatchProperties(__unused Dictionary *properties)
	{
		return true;
	}


	void Service::StartMatching()
	{
		DoMatch();
	}

	void Service::DoMatch()
	{
		EnumerateClassesForProvider(this, [this](MetaClass *meta, Dictionary *properties) {

			if(MatchProperties(properties))
			{
				Service *service = static_cast<Service *>(meta->Alloc());
				service = service->InitWithProperties(properties);

				if(service)
				{
					PublishService(service);
					return true;
				}
			}

			return false;

		});
	}


	void Service::PublishService(Service *service)
	{
		AttachChild(service);
	}
	void Service::AttachToParent(RegistryEntry *parent)
	{
		RegistryEntry::AttachToParent(parent);
		Start();
	}
}
