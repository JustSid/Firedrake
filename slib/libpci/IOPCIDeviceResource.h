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
#ifndef _IOPCIDEVICERESOURCE_H_
#define _IOPCIDEVICERESOURCE_H_

#include <core/IOObject.h>

namespace IO
{
	class PCIDevice;
	class PCIDeviceResource : public Object
	{
	public:
		friend class PCIDevice;

		enum class Type
		{
			Memory,
			IOSpace
		};

		PCIDevice *GetDevice() const;
		Type GetType() const;

		uint8_t GetBar();

		uint32_t GetAddress() const;
		size_t GetSize() const;

	protected:
		PCIDeviceResource *Init(PCIDevice *device, uint32_t config, uint8_t bar);

	private:
		PCIDevice *_device;
		Type _type;

		uint32_t _address;
		size_t _size;
		uint8_t _bar;

		IODeclareMeta(PCIDeviceResource)
	};
}

#endif /* _IOPCIDEVICERESOURCE_H_ */
