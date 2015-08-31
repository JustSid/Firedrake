//
//  IOPCIDeviceResource.h
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

#include "IOPCIDeviceResource.h"
#include "IOPCIDevice.h"

namespace IO
{
	IODefineMeta(PCIDeviceResource, Object)

	PCIDeviceResource *PCIDeviceResource::Init(PCIDevice *device, uint32_t config, uint8_t bar)
	{
		if(!Object::Init())
			return nullptr;

		_device = device;
		_bar = bar;
		_type = (config & 0x1) ? Type::IOSpace : Type::Memory;

		size_t offset = 0x10 + (_bar * 4);

		switch(_type)
		{
			case Type::Memory:
			{
				switch((config >> 1) & 0x3)
				{
					case 0x0:
						_address = config & 0xfffffff0;
						break;
					case 0x1:
						_address = config & 0xfff0;
						break;
					case 0x2:
					default:
						panic("64Bit memory BAR not supported");
				}

				// Calculate the size
				device->WriteConfig32(offset, 0xffffffff);
				uint32_t result = device->ReadConfig32(offset);
				device->WriteConfig32(offset, config);

				_size = (~(result & 0xfffffff0)) + 1;
				break;
			}

			case Type::IOSpace:
			{
				panic("Oi, mate");
			}
		}

		return this;
	}

	PCIDevice *PCIDeviceResource::GetDevice() const
	{
		return _device;
	}
	PCIDeviceResource::Type PCIDeviceResource::GetType() const
	{
		return _type;
	}

	uint8_t PCIDeviceResource::GetBar()
	{
		return _bar;
	}

	uint32_t PCIDeviceResource::GetAddress() const
	{
		return _address;
	}
	size_t PCIDeviceResource::GetSize() const
	{
		return _size;
	}
}
