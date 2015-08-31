//
//  IOPCIProvider.cpp
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

#include "IOPCIProvider.h"
#include <core/IONumber.h>
#include <port.h>

namespace IO
{
	IODefineMeta(PCIProvider, Service)

	void PCIProvider::InitialWakeUp(MetaClass *meta)
	{
		if(meta == PCIProvider::GetMetaClass())
		{
			Dictionary *properties = Dictionary::Alloc()->Init();
			String *className = String::Alloc()->InitWithCString("IO::Resources", false);

			properties->SetObjectForKey(className, kServiceProviderMatchKey);

			className->Release();

			RegisterService(meta, properties);
			properties->Release();
		}

		Service::InitialWakeUp(meta);
	}

	PCIProvider *PCIProvider::InitWithProperties(Dictionary *properties)
	{
		if(!Service::InitWithProperties(properties))
			return nullptr;

		_foundDevices = Dictionary::Alloc()->Init();

		return this;
	}

	bool PCIProvider::MatchProperties(Dictionary *properties)
	{
		Number *vendorID = properties->GetObjectForKey<Number>(kPCIDeviceVendorIDKey);
		Number *deviceID = properties->GetObjectForKey<Number>(kPCIDeviceDeviceIDKey);

		if(!vendorID || !deviceID)
			return false;

		Number *lookup = Number::Alloc()->InitWithUint32((vendorID->GetUint16Value() << 16) | deviceID->GetUint16Value());
		bool result = (_foundDevices->GetObjectForKey(lookup) != nullptr);
		lookup->Release();

		return result;
	}

	void PCIProvider::PublishService(Service *service)
	{
		PCIDevice *pciDevice = service->Downcast<PCIDevice>();
		if(pciDevice)
		{
			Number *lookup = Number::Alloc()->InitWithUint32((pciDevice->GetVendorID() << 16) | pciDevice->GetDeviceID());
			Number *location = _foundDevices->GetObjectForKey<Number>(lookup);
			lookup->Release();

			uint8_t device = static_cast<uint8_t>(location->GetUint16Value() & 0xff);
			uint8_t bus = static_cast<uint8_t>(location->GetUint16Value() >> 8);

			pciDevice->PrepareWithPublish(bus, device, 0);
		}

		Service::PublishService(service);
	}

	void PCIProvider::ProbeDevice(uint8_t bus, uint8_t device)
	{
		uint32_t config;

		{
			uint32_t address = ((uint32_t)(1 << 31)) | (bus << 16) | (device << 11);
			outl(0xcf8, address);
			config = inl(0xcfc);
		}

		uint16_t vendorID = config & 0xffff;
		uint16_t deviceID = (config >> 16) & 0xffff;

		if(vendorID != 0xffff)
		{
			Number *deviceKey = Number::Alloc()->InitWithUint32((vendorID << 16) | deviceID);
			Number *locationKey = Number::Alloc()->InitWithUint16((bus << 8) | device);

			_foundDevices->SetObjectForKey(locationKey, deviceKey);

			deviceKey->Release();
			locationKey->Release();

#if 0
			kprintf("Found PCI device: <%x, %x>\n", vendorID, deviceID);
#endif
		}
	}

	void PCIProvider::Start()
	{
		Service::Start();

		for(int i = 0; i < 256; i ++)
		{
			for(int j = 0; j < 32; j ++)
			{
				ProbeDevice(i, j);
			}
		}

		RegisterProvider();
	}
}

bool _pci_start(__unused kmod_t *mod)
{
	return true;
}
void _pci_stop(__unused kmod_t *mod)
{}

kern_start_stub(_pci_start)
kern_stop_stub(_pci_stop)
