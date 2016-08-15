//
//  IOPCIDevice.h
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

#ifndef _IOPCISERVICE_H_
#define _IOPCISERVICE_H_

#include <core/IOObject.h>
#include <service/IOService.h>
#include "IOPCIDeviceResource.h"

namespace IO
{
	extern String *kPCIDeviceVendorIDKey;
	extern String *kPCIDeviceDeviceIDKey;
	extern String *kPCIDeviceClassIDKey;
	extern String *kPCIDeviceSubclassIDKey;

	class PCIProvider;

	class PCIDevice : public Service
	{
	public:
		enum class Type
		{
			Generic,
			PCIToPCIBridge,
			PCIToCardBus
		};

		friend class PCIProvider;

		static void InitialWakeUp(MetaClass *meta);

		PCIDevice *InitWithProperties(Dictionary *properties) override;
		void Dealloc() override;

		void Start() override;

		uint16_t GetVendorID() const;
		uint16_t GetDeviceID() const;

		uint8_t ReadConfig8(size_t offset) const;
		uint16_t ReadConfig16(size_t offset) const;
		uint32_t ReadConfig32(size_t offset) const;

		void WriteConfig8(size_t offset, uint8_t val);
		void WriteConfig16(size_t offset, uint16_t val);
		void WriteConfig32(size_t offset, uint32_t val);

		PCIDeviceResource *GetResourceWithIndex(size_t index) const;

	private:
		void PrepareWithPublish(uint8_t bus, uint8_t device, uint8_t function);

		Type _type;
		size_t _config;

		uint16_t _vendorID;
		uint16_t _deviceID;

		uint8_t _bus;
		uint8_t _device;
		uint8_t _function;

		uint8_t _BIST;
		uint8_t _latencyTimer;
		uint8_t _cacheLineSize;

		uint8_t _interruptPin;
		uint8_t _interruptLine;

		Array *_resources;

		IODeclareMeta(PCIDevice)
	};
}

#endif /* _IOPCISERVICE_H */
