//
//  PCIProvider.cpp
//  libPCI
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

#include "PCIProvider.h"
#include "PCIDevice.h"

IOString *PCIDeviceIdentifier;
IOString *PCIDeviceFamily;

IOString *PCIDevicePropertyVendorID;
IOString *PCIDevicePropertyDeviceID;
IOString *PCIDevicePropertyClassID;
IOString *PCIDevicePropertySubclassID;

#ifdef super
#undef super
#endif
#define super IOModule

IORegisterModule(PCIProvider);
IORegisterClass(PCIProvider, super)

PCIProvider *PCIProvider::initWithKmod(kern_module_t *kmod)
{
	if(!super::initWithKmod(kmod))
		return 0;

	_lock = KERN_SPINLOCK_INIT;
	_devices = IODictionary::alloc()->init();

	if(_devices == 0)
	{
		release();
		return 0;
	}

	return this;
}

void PCIProvider::free()
{
	_devices->release();
	super::free();
}



uint32_t PCIProvider::readConfig(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	uint32_t address = ((uint32_t)(1 << 31)) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);

	outl(0xCF8, address);
	return inl(0xCFC);
}

void PCIProvider::checkDevice(uint8_t bus, uint8_t device)
{
	uint8_t function = 0;
	uint32_t info = readConfig(bus, device, function, 0);

	uint16_t vendorID = (uint16_t)(info);
	uint16_t deviceID = (uint16_t)(info >> 16);

	if(vendorID != 0xFFFF)
	{
		IONumber *lookup = IONumber::alloc()->initWithInt16((int16_t)(bus << 8) | (device));
		PCIDevice *pcidevice = (PCIDevice *)_devices->objectForKey(lookup);

		if(!pcidevice)
		{
			pcidevice = PCIDevice::alloc()->initWithInfo(bus, device, function, vendorID, deviceID)->autorelease();
			if(pcidevice)
			{
				bool result = publishService(pcidevice);
				if(result)
				{
					_devices->setObjectForKey(pcidevice, lookup);
					IOLog("Discovered %@", pcidevice);
				}
			}
		}

		lookup->release();
	}
}

void PCIProvider::requestProbe()
{
	kern_spinlock_lock(&_lock);

	for(int bus=0; bus<256; bus++)
	{
		for(int device=0; device<32; device++)
		{
			checkDevice(bus, device);
		}
	}

	kern_spinlock_unlock(&_lock);
}


bool PCIProvider::publish()
{
	PCIDeviceIdentifier = IOString::alloc()->initWithCString("PCIDeviceIdentifier");
	PCIDeviceFamily     = IOString::alloc()->initWithCString("PCIDeviceFamily");

	PCIDevicePropertyVendorID   = IOString::alloc()->initWithCString("PCIDevicePropertyVendorID");
	PCIDevicePropertyDeviceID   = IOString::alloc()->initWithCString("PCIDevicePropertyDeviceID");
	PCIDevicePropertyClassID    = IOString::alloc()->initWithCString("PCIDevicePropertyClassID");
	PCIDevicePropertySubclassID = IOString::alloc()->initWithCString("PCIDevicePropertySubclassID");
	
	return super::publish();
}

void PCIProvider::unpublish()
{
	PCIDeviceIdentifier->release();
	PCIDeviceFamily->release();
	PCIDevicePropertyVendorID->release();
	PCIDevicePropertyClassID->release();
	PCIDevicePropertySubclassID->release();
}
