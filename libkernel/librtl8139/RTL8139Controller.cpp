//
//  RTL8139Controller.cpp
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

#include "RTL8139Controller.h"
#include "RTL8139Constants.h"

#ifdef super
#undef super
#endif
#define super IOEthernetController

IORegisterClass(RTL8139Controller, super);

bool RTL8139Controller::start(IOProvider *provider)
{
	if(!super::start(provider))
		return false;

	_device = (PCIDevice *)provider;
	_lock = KERN_SPINLOCK_INIT;

	IOArray *resources = _device->resources();
	for(uint32_t i=0; i<resources->count(); i++)
	{
		PCIDeviceResource *resource = (PCIDeviceResource *)resources->objectAtIndex(i);
		
		if(resource->type() == kPCIDeviceResourceTypeIOPort)
			_portBase = resource->address();
	}
	
	writeRegister8(RTL_CONFIG0, 0x0);

	// Setup interrupts and reset the card
	_interruptSource = _device->registerForInterrupt(this, IOMemberFunctionCast(IOInterruptEventSource::Action, this, &RTL8139Controller::handleInterrupt));
	runLoop()->addEventSource(_interruptSource);

	if(!reset())
	{
		IOLog("Couldn't reset card");
		return false;
	}

	_macAddress = readRegister32(RTL_IDR0) | ((uint64_t)readRegister32(RTL_IDR4) << 32);
	_queuedData = IOArray::alloc()->init();

	// Enable Rx/Tx
	writeRegister8(RTL_CM, R_CM_TE | R_CM_RE);

	// Initialize RCR/TCR
	writeRegister32(RTL_RCR, R_RCR_MXDMA | R_RCR_AB | R_RCR_APM);
	writeRegister32(RTL_TCR, R_TCR_IFG | R_TCR_MXDMA);

	// Interrupt mask
	writeRegister16(RTL_ISR, 0);
	writeRegister16(RTL_IMR, 0xFFFF);

	// Get some buffers up in here
	_receiveRegion  = IODMARegion::alloc()->initWithRequest(IODMARegion::RequestContiguous | IODMARegion::Request32bitLimit, 3);
	_transmitRegion = IODMARegion::alloc()->initWithRequest(IODMARegion::RequestContiguous | IODMARegion::Request32bitLimit, 3);

	_transmitBuffer = (uint8_t *)_transmitRegion->virtualRegion();
	_receiveBuffer  = (uint8_t *)_receiveRegion->virtualRegion();
	_receiveBufferOffset = 0;

	writeRegister32(RTL_RBSTART, _receiveRegion->physicalRegion());

	return true;
}

void RTL8139Controller::stop(IOProvider *provider)
{
	runLoop()->removeEventSource(_interruptSource);

	_receiveRegion->release();
	_transmitRegion->release();
	_queuedData->release();

	super::stop(provider);
}

bool RTL8139Controller::reset()
{
	writeRegister8(RTL_CM, R_CM_RST);

	IOThread::currentThread()->sleep(10);

	return !(readRegister8(RTL_CM) & R_CM_RST);
}


void RTL8139Controller::sendData(IOData *data)
{
	if(!kern_spinlock_tryLock(&_lock))
	{
		_queuedData->addObject(data);
		return;
	}

	size_t size = data->length();
	memcpy(_transmitBuffer, data->bytes(), size);

	uint32_t currentBuffer = _currentBuffer;
	_currentBuffer ++;
	_currentBuffer %= 4;

	writeRegister32(RTL_TSAD0 + (4 * currentBuffer), _transmitRegion->physicalRegion());
	writeRegister32(RTL_TSD0 + (4 * currentBuffer), size);
}


void RTL8139Controller::handleInterrupt(IOInterruptEventSource *UNUSED(source), uint32_t UNUSED(missed))
{
	uint32_t isr = readRegister32(RTL_ISR);
	uint32_t cleared = 0;

	if(isr & R_ISR_ROK)
	{
		IOLog("Received packet");
		cleared |= R_ISR_ROK;
	}

	if(isr & R_ISR_TOK)
	{
		IOLog("Transmitted packet");
		cleared |= R_ISR_TOK;
	}

	if(cleared != isr)
	{
		IOLog("Unhandled interrupt case!");
	}

	writeRegister32(RTL_ISR, cleared);
}




IOString *RTL8139Controller::description() const
{
	uint8_t macByte0 = (uint8_t)(_macAddress >> 40);
	uint8_t macByte1 = (uint8_t)(_macAddress >> 32);
	uint8_t macByte2 = (uint8_t)(_macAddress >> 24);
	uint8_t macByte3 = (uint8_t)(_macAddress >> 16);
	uint8_t macByte4 = (uint8_t)(_macAddress >> 8);
	uint8_t macByte5 = (uint8_t)(_macAddress);

	return IOString::withFormat("<RTL8139 controller:%p> MAC address: %02x:%02x:%02x:%02x:%02x:%02x, IRQ: %i", this, (int)macByte0, (int)macByte1, (int)macByte2, (int)macByte3, (int)macByte4,(int)macByte5, _interruptSource->interrupt());
}

void RTL8139Controller::writeRegister8(uint8_t reg, uint8_t value)
{
	outb(_portBase + reg, value);
}
void RTL8139Controller::writeRegister16(uint8_t reg, uint16_t value)
{
	outw(_portBase + reg, value);
}
void RTL8139Controller::writeRegister32(uint8_t reg, uint32_t value)
{
	outl(_portBase + reg, value);
}

uint8_t RTL8139Controller::readRegister8(uint8_t reg)
{
	return inb(_portBase + reg);
}
uint16_t RTL8139Controller::readRegister16(uint8_t reg)
{
	return inw(_portBase + reg);
}
uint32_t RTL8139Controller::readRegister32(uint8_t reg)
{
	return inl(_portBase + reg);
}
