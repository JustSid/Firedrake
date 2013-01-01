//
//  PCIDevice.h
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

#ifndef _PCISERVICE_H_
#define _PCISERVICE_H_

#include <libio/libio.h>
#include "PCIClass.h"
#include "PCIDeviceResource.h"

// offset | bits 31 - 24   |   bits 23 - 16   |   bits 15 - 8   |   bits 7 - 0
//   00     Device ID      |                  |   Vendor ID  
//   04     Status                            |   Command  
//   08     Class Code     |   Subclass       |   Prog IF       |   Revision
//   0C     BIST           |   Header type    |   Latency timer |   Cache line size

typedef enum
{
	kPCIDeviceTypeGeneric,
	kPCIDeviceTypePCItoPCIBridge,
	kPCIDeviceTypePCItoCardBusBridge
} PCIDeviceType;

class PCIDevice : public IOService
{
friend class PCIDeviceResource;
public:
	virtual PCIDevice *initWithInfo(uint8_t bus, uint8_t device, uint8_t function, uint16_t vendorID, uint16_t deviceID);

	virtual IOString *description() const;

	virtual bool start(IOProvider *provider);
	virtual void stop(IOProvider *provider);

	bool canPerformSelfCheck();
	bool performSelfCheck();

	PCIDeviceType type() const;

	IOArray *resources() const
	{ return _resources; }


	void setCommand(uint16_t command);
	uint16_t command();
	uint16_t status();

	IOInterruptEventSource *registerForInterrupt(IOObject *owner, IOInterruptEventSource::Action action, uint32_t interrupt=0);
	void unregisterFromInterrupt();

	uint32_t readConfig(uint8_t offset);
	void writeConfig(uint8_t offset, uint32_t value);

	// Should be treated as read-only
	uint16_t _vendorID;
	uint16_t _deviceID;

	uint8_t _classID;
	uint8_t _subclassID;

	uint8_t _BIST;
	uint8_t _headerType;
	uint8_t _latencyTimer;
	uint8_t _cacheLineSize;

	uint8_t _interruptPin;
	uint8_t _interruptLine;

private:
	virtual void free();
	virtual void handleInterrupt() {}

	bool scanForResources();

	IOArray *_resources;
	IOService *_controller;
	IOInterruptEventSource *_interruptSource;

	uint8_t _bus;
	uint8_t _device;
	uint8_t _function;

	uint32_t _caddress;

	IODeclareClass(PCIDevice)
};

#endif /* _PCISERVICE_H_ */
