//
//  PCIProvider.h
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

#ifndef _PCIPROVIDER_H_
#define _PCIPROVIDER_H_

#include <libio/libio.h>

extern IOString *PCIDeviceIdentifier;
extern IOString *PCIDeviceFamily;

extern IOString *PCIDevicePropertyVendorID; // IONumber::UInt16
extern IOString *PCIDevicePropertyDeviceID; // IONumber::UInt16
extern IOString *PCIDevicePropertyClassID; // IONumber::UInt8
extern IOString *PCIDevicePropertySubclassID; // IONumber::UInt8

class PCIProvider : public IOModule
{
public:
	virtual PCIProvider *initWithKmod(kern_module_t *kmod);
	virtual void requestProbe();

	virtual bool publish();
	virtual void unpublish();

private:
	virtual void free();

	uint32_t readConfig(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
	void checkDevice(uint8_t bus, uint8_t device);

	IODictionary *_devices;
	kern_spinlock_t _lock;
	bool _firstRun;

	IODeclareClass(PCIProvider)
};

#endif /* _PCIPROVIDER_H_ */
