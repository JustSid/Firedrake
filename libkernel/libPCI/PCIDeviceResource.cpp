//
//  PCIDeviceResource.cpp
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

#include "PCIDeviceResource.h"
#include "PCIDevice.h"

#ifdef super
#undef super
#endif
#define super IOObject

IORegisterClass(PCIDeviceResource, super);

PCIDeviceResource *PCIDeviceResource::initWithData(PCIDevice *device, PCIDeviceResourceType type, uint8_t bar)
{
	if(!super::init())
		return 0;

	uint32_t offset = 0x10 + (bar * 4);

	_bar = bar;
	_type = type;

	switch(type)
	{
		case kPCIDeviceResourceTypeMemory:
		{
			uint32_t config = device->readConfig(offset);
			uint32_t value;

			_prefetchable = (((config >> 3) & 0x1) == 1);

			switch((config >> 1) & 0x3)
			{
				case 0x0:
				{
					device->writeConfig(offset, 0xFFFFFFF0);
					value = device->readConfig(offset) & 0xFFFFFFF0;
					device->writeConfig(offset, config);

					if(value == 0)
					{
						release();
						return 0;
					}

					_address = (config & ~0xF);
					_size = (~value | 0xF) + 1;

					break;
				}
					
				case 0x1:
					panic("20Bit memory BARs are unsupported!");
					break;

				case 0x2:
					panic("64Bit memory BARs are unsupported!");
					break;
			}

			break;
		}

		case kPCIDeviceResourceTypeIOPort:
		{
			uint32_t config = device->readConfig(offset);
			device->writeConfig(offset, 0xFFFFFFFC);

			uint32_t value = device->readConfig(offset) & 0xFFFFFFFC;
			device->writeConfig(offset, config);

			if(value == 0)
			{
				release();
				return 0;
			}

			_address = (config & ~0xF);
			_size = (~value | 0xF) + 1;
			break;
		}
	}

	return this;
}

IOString *PCIDeviceResource::description() const
{
	switch(_type)
	{
		case kPCIDeviceResourceTypeMemory:
			return IOString::withFormat("32Bit %s memory resource, address %p, size %ibytes", _prefetchable ? "prefetchable" : "non-prefetchable", _address, _size);

		case kPCIDeviceResourceTypeIOPort:
			return IOString::withFormat("I/O Port resource, start %p, size %i", _address, _size);		
	}
}
