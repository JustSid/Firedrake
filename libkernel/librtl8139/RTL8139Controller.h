//
//  RTL8139Controller.h
//  libRTL8139
//
//  Created by Sidney
//  Copyright (c) 2013 by Sidney
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

#ifndef _RTL8139SERVICE_H_
#define _RTL8139SERVICE_H_

#include <libio/libio.h>
#include <libPCI/PCIDevice.h>

class RTL8139Controller : public IOEthernetController
{
public:
	virtual bool start(IOProvider *provider);
	virtual void stop(IOProvider *provider);

	virtual IOString *description() const;

	void sendData(IOData *data);
	bool reset();

private:
	void handleInterrupt(IOInterruptEventSource *source, uint32_t interrupt);

	void writeRegister8(uint8_t reg, uint8_t value);
	void writeRegister16(uint8_t reg, uint16_t value);
	void writeRegister32(uint8_t reg, uint32_t value);

	uint8_t readRegister8(uint8_t reg);
	uint16_t readRegister16(uint8_t reg);
	uint32_t readRegister32(uint8_t reg);

	kern_spinlock_t _lock;

	IODMARegion *_receiveRegion;
	IODMARegion *_transmitRegion;

	PCIDevice *_device;
	IOInterruptEventSource *_interruptSource;
	IOArray *_queuedData;

	uint32_t _portBase;
	uint64_t _macAddress;

	uint32_t _currentBuffer;

	uint8_t *_transmitBuffer;
	uint8_t *_receiveBuffer;
	size_t _receiveBufferOffset;

	IODeclareClass(RTL8139Controller)
};

#endif /* _RTL8139SERVICE_H_ */
