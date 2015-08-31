//
//  IOPCIDevice.cpp
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

#include "IOPCIDevice.h"
#include <core/IONumber.h>
#include <port.h>

namespace IO
{
	String *kPCIDeviceVendorIDKey;
	String *kPCIDeviceDeviceIDKey;
	String *kPCIDeviceClassIDKey;
	String *kPCIDeviceSubclassIDKey;

	IODefineMeta(PCIDevice, Service)

	void PCIDevice::InitialWakeUp(MetaClass *meta)
	{
		if(meta == PCIDevice::GetMetaClass())
		{
			kPCIDeviceVendorIDKey = String::Alloc()->InitWithCString("kPCIDeviceVendorIDKey", false);
			kPCIDeviceDeviceIDKey = String::Alloc()->InitWithCString("kPCIDeviceDeviceIDKey", false);
			kPCIDeviceClassIDKey = String::Alloc()->InitWithCString("kPCIDeviceClassIDKey", false);
			kPCIDeviceSubclassIDKey = String::Alloc()->InitWithCString("kPCIDeviceSubclassIDKey", false);
		}

		Service::InitialWakeUp(meta);
	}

	PCIDevice *PCIDevice::InitWithProperties(Dictionary *properties)
	{
		if(!Service::InitWithProperties(properties))
			return nullptr;

		Number *vendor = properties->GetObjectForKey<Number>(kPCIDeviceVendorIDKey);
		Number *device = properties->GetObjectForKey<Number>(kPCIDeviceDeviceIDKey);

		_vendorID = vendor->GetUint16Value();
		_deviceID = device->GetUint16Value();
		_resources = Array::Alloc()->Init();

		return this;
	}

	void PCIDevice::Dealloc()
	{
		_resources->Release();

		Service::Dealloc();
	}

	void PCIDevice::Start()
	{
		Service::Start();

		_config = ((uint32_t)(1 << 31)) | (_bus << 16) | (_device << 11) | (_function << 8);

		uint32_t config;

		// Read the BIST, header type, etc
		config = ReadConfig32(0xc);

		_BIST = static_cast<uint8_t>((config >> 24) & 0xff);
		_latencyTimer = static_cast<uint8_t>((config >> 8) & 0xff);
		_cacheLineSize = static_cast<uint8_t>(config & 0xff);
		uint8_t header = static_cast<uint8_t>((config >> 16) & 0xff);

		switch(header)
		{
			case 0:
				_type = Type::Generic;
				break;
			case 1:
				_type = Type::PCIToPCIBridge;
				break;
			case 2:
				_type = Type::PCIToCardBus;
				break;
			case 3:
				panic("Unknown PCI header");
		}

		// Interrupts
		config = ReadConfig32(0x3c);

		_interruptPin = static_cast<uint8_t>((config >> 8) & 0xff);
		_interruptLine = static_cast<uint8_t>(config & 0xff);

		// Scan for present BARs
		for(size_t i = 0; i < 6; i ++)
		{
			size_t offset = 0x10 + (i * 4);
			uint32_t config = ReadConfig32(offset);

			if(config == 0)
				continue;

			PCIDeviceResource *resource = PCIDeviceResource::Alloc()->Init(this, config, static_cast<uint8_t>(i));
			_resources->AddObject(resource);
			resource->Release();
		}
	}

	PCIDeviceResource *PCIDevice::GetResourceWithIndex(size_t index) const
	{
		size_t size = _resources->GetCount();
		for(size_t i = 0; i < size; i ++)
		{
			PCIDeviceResource *resource = _resources->GetObjectAtIndex<PCIDeviceResource>(i);
			if(resource->GetBar() == index)
				return resource;
		}

		return nullptr;
	}

	void PCIDevice::PrepareWithPublish(uint8_t bus, uint8_t device, uint8_t function)
	{
		_bus = bus;
		_device = device;
		_function = function;
	}


	uint16_t PCIDevice::GetVendorID() const
	{
		return _vendorID;
	}
	uint16_t PCIDevice::GetDeviceID() const
	{
		return _deviceID;
	}


	uint8_t PCIDevice::ReadConfig8(size_t offset) const
	{
		size_t address = _config | (offset & 0xfc);

		outl(0xcf8, address);
		return inb(0xcfc + (offset & 1));
	}
	uint16_t  PCIDevice::ReadConfig16(size_t offset) const
	{
		size_t address = _config | (offset & 0xfc);

		outl(0xcf8, address);
		return inw(0xcfc + (offset & 2));
	}
	uint32_t PCIDevice::ReadConfig32(size_t offset) const
	{
		size_t address = _config | (offset & 0xfc);

		outl(0xcf8, address);
		return inl(0xcfc);
	}

	void PCIDevice::WriteConfig8(size_t offset, uint8_t val)
	{
		size_t address = _config | (offset & 0xfc);

		outl(0xcf8, address);
		outw(0xcfc + (offset & 1), val);

	}
	void PCIDevice::WriteConfig16(size_t offset, uint16_t val)
	{
		size_t address = _config | (offset & 0xfc);

		outl(0xcf8, address);
		outw(0xcfc + (offset & 2), val);
	}
	void PCIDevice::WriteConfig32(size_t offset, uint32_t val)
	{
		size_t address = _config | (offset & 0xfc);

		outl(0xcf8, address);
		outl(0xcfc, val);
	}
}
