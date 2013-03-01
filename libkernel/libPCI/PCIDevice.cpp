//
//  PCIDevice.cpp
//  libPCI
//
//  Created by Sidney Just
//  Copyright (c) 2013 by Sidney Just
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

#include <libkernel/interrupts.h>
#include "PCIDevice.h"
#include "PCIProvider.h"

#ifdef super
#undef super
#endif
#define super IOService

IORegisterClass(PCIDevice, super);

PCIDevice *PCIDevice::initWithInfo(uint8_t bus, uint8_t device, uint8_t function, uint16_t vendorID, uint16_t deviceID)
{
	if(!super::init())
		return 0;

	_bus = bus;
	_device = device;
	_function = function;

	_caddress = ((uint32_t)(1 << 31)) | (bus << 16) | (device << 11) | (function << 8);

	_vendorID = vendorID;
	_deviceID = deviceID;

	_resources = IOArray::alloc()->init();
	if(!_resources)
	{
		release();
		return 0;
	}


	uint32_t config;

	// Read class, subclass, prog IF, revision
	config = readConfig(0x8);

	_classID = (uint8_t)(config >> 24);
	_subclassID = (uint8_t)(config >> 16);

	// Read BIST, header type, latency timer and cache line size
	config = readConfig(0xC);

	_BIST = (uint8_t)(config >> 24);
	_headerType = (uint8_t)(config >> 16);
	_latencyTimer = (uint8_t)(config >> 8);
	_cacheLineSize = (uint8_t)(config);

	// Bridge control, interrupt pin interrupt line
	config = readConfig(0x3C);
	_interruptPin = (uint8_t)(config >> 8);
	_interruptLine = (uint8_t)(config);

	if(!scanForResources())
	{
		release();
		return 0;
	}

	return this;
}

void PCIDevice::free()
{
	_resources->release();
	super::free();
}

IOString *PCIDevice::description() const
{
	IOString *desc = IOString::withFormat("<%s:%p> {%x, %x}, class: %x, subclass: %x", this->symbol()->name()->CString(), this, _vendorID, _deviceID, _classID, _subclassID);
	return desc;
}

// -------
// Bootstrap
// -------

bool PCIDevice::start(IOProvider *provider)
{
	if(!super::start(provider))
		return false;

	IODictionary *properties = IODictionary::alloc()->init()->autorelease();
	properties->setObjectForKey(IONumber::withUInt16(_vendorID), PCIDevicePropertyVendorID);
	properties->setObjectForKey(IONumber::withUInt16(_deviceID), PCIDevicePropertyDeviceID);
	properties->setObjectForKey(IONumber::withUInt8(_classID), PCIDevicePropertyClassID);
	properties->setObjectForKey(IONumber::withUInt8(_subclassID), PCIDevicePropertySubclassID);

	IODictionary *attributes = IOService::createMatchingDictionary(PCIDeviceFamily, properties);

	_controller = findMatchingService(attributes);
	if(!_controller)
		return false;

	if(!publishService(_controller))
		return false;

	return true;
}

void PCIDevice::stop(IOProvider *provider)
{
	unpublishService(_controller);
	_controller = 0;

	super::stop(provider);
}

// -------
// BIST
// -------

bool PCIDevice::canPerformSelfCheck()
{
	return (_BIST & (1 << 7));
}

bool PCIDevice::performSelfCheck()
{
	if(canPerformSelfCheck())
	{
		_BIST |= (1 << 6);

		uint32_t config = (_BIST << 24) | (_headerType << 16) | (_latencyTimer << 16) | (_cacheLineSize);
		writeConfig(0xC, config);

		do {
			IOThread::yield();

			config = readConfig(0xC);
			_BIST = (uint8_t)(config >> 24);

		} while(_BIST & (1 << 6));

		uint8_t result = (_BIST & 0x7);
		return (result == 0);
	}

	return true;
}

// -------
// Abstraction methods
// -------

PCIDeviceType PCIDevice::type() const
{
	uint8_t type = _headerType & 0x3;
	switch(type)
	{
		case 0x0:
			return kPCIDeviceTypeGeneric;
			break;

		case 0x1:
			return kPCIDeviceTypePCItoPCIBridge;
			break;

		case 0x2:
			return kPCIDeviceTypePCItoCardBusBridge;
			break;

		default:
			panic("Unknown PCI device type");
	}
}

void PCIDevice::setCommand(uint16_t command)
{
	uint32_t config = readConfig(0x4);
	uint16_t status = (uint16_t)(config >> 16);

	config = ((status << 16 ) | command);

	writeConfig(0x4, config);
}

uint16_t PCIDevice::status()
{
	uint32_t config = readConfig(0x4);
	return (uint16_t)(config >> 16);
}

uint16_t PCIDevice::command()
{
	uint32_t config = readConfig(0x4);
	return (uint16_t)(config);
}

// -------
// Device handling
// -------

bool PCIDevice::scanForResources()
{
	_resources->removeAllObjects();

	uint8_t barCount = (type() == kPCIDeviceTypeGeneric) ? 6 : 2;

	for(uint8_t i=0; i<barCount; i++)
	{
		uint32_t offset = 0x10 + (i * 4);
		uint32_t config = readConfig(offset);

		if((config & 0x1) == 0)
		{
			PCIDeviceResource *resource = PCIDeviceResource::alloc()->initWithData(this, kPCIDeviceResourceTypeMemory, i);
			if(resource)
			{
				_resources->addObject(resource);
				resource->release();
			}
		}
		else
		{
			PCIDeviceResource *resource = PCIDeviceResource::alloc()->initWithData(this, kPCIDeviceResourceTypeIOPort, i);
			if(resource)
			{
				_resources->addObject(resource);
				resource->release();
			}
		}
	}

	return true;
}

// -------
// Interrupt handling
// -------

IOInterruptEventSource *PCIDevice::registerForInterrupt(IOObject *owner, IOInterruptEventSource::Action action, uint32_t interrupt)
{
	_interruptSource = IOInterruptEventSource::alloc()->initWithInterrupt(interrupt, owner, action);

	if(_interruptSource)
	{
		_interruptLine = (uint8_t)_interruptSource->interrupt();

		uint32_t config = readConfig(0x3C);
		uint16_t status = (uint16_t)(config >> 16);

		config = ((status << 16 ) | (_interruptPin << 8) | _interruptLine);
		writeConfig(0x3C, config);
	}

	return _interruptSource;
}

void PCIDevice::unregisterFromInterrupt()
{
	if(_interruptSource)
	{
		_interruptLine = 0xFF;

		uint32_t config = readConfig(0x3C);
		uint16_t status = (uint16_t)(config >> 16);

		config = ((status << 16 ) | (_interruptPin << 8) | _interruptLine);
		writeConfig(0x3C, config);

		_interruptSource->release();
		_interruptSource = 0;
	}
}

// -------
// Register reading/writing
// -------

uint32_t PCIDevice::readConfig(uint8_t offset)
{
	uint32_t address = _caddress | (offset & 0xFC);
	
	outl(0xCF8, address);
	return inl(0xCFC);
}

void PCIDevice::writeConfig(uint8_t offset, uint32_t value)
{
	uint32_t address = _caddress | (offset & 0xFC);
	
	outl(0xCF8, address);
	outl(0xCFC, value);
}
