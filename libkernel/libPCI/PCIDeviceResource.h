//
//  PCIDeviceResource.h
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

#ifndef _PCIDEVICERESOURCE_H_
#define _PCIDEVICERESOURCE_H_

#include <libio/libio.h>

typedef enum
{
	kPCIDeviceResourceTypeMemory,
	kPCIDeviceResourceTypeIOPort
} PCIDeviceResourceType;

class PCIDevice;
class PCIDeviceResource : public IOObject
{
public:
	PCIDeviceResource *initWithData(PCIDevice *device, PCIDeviceResourceType type, uint8_t bar);

	virtual IOString *description() const;

	PCIDeviceResourceType type() const
	{ return _type; }

	uint8_t bar() const
	{ return _bar; }

	uint32_t address() const
	{ return _address; }
	uint32_t start() const
	{ return _address; }
	size_t size() const
	{ return _size; }

private:
	PCIDevice *_device;
	PCIDeviceResourceType _type;

	bool _prefetchable;

	uint32_t _address;
	size_t _size;
	uint8_t _bar;

	IODeclareClass(PCIDeviceResource)
};

#endif /* _PCIDEVICERESOURCE_H_ */
