//
//  QMVBEDevice.cpp
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

#include "QMVBEDevice.h"
#include <core/IONumber.h>
#include <port.h>

// https://github.com/avikivity/qemu/blob/master/kvm/vgabios/vbe_display_api.txt

#define PREFERRED_VY 4096
#define PREFERRED_B 32

#define VBE_DISPI_IOPORT_INDEX          0x01ce
#define VBE_DISPI_IOPORT_DATA           0x01cf

#define VBE_DISPI_INDEX_ID              0x0
#define VBE_DISPI_INDEX_XRES            0x1
#define VBE_DISPI_INDEX_YRES            0x2
#define VBE_DISPI_INDEX_BPP             0x3
#define VBE_DISPI_INDEX_ENABLE          0x4
#define VBE_DISPI_INDEX_BANK            0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH      0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT     0x7
#define VBE_DISPI_INDEX_X_OFFSET        0x8
#define VBE_DISPI_INDEX_Y_OFFSET        0x9

#define VBE_DISPI_ID0                   0xb0c0
#define VBE_DISPI_ID1                   0xb0c1
#define VBE_DISPI_ID2                   0xb0c2
#define VBE_DISPI_ID3                   0xb0c3
#define VBE_DISPI_ID4                   0xb0c4

#define VBE_DISPI_DISABLED              0x00
#define VBE_DISPI_ENABLED               0x01
#define VBE_DISPI_GETCAPS               0x02
#define VBE_DISPI_LFB_ENABLED           0x40
#define VBE_DISPI_NOCLEARMEM            0x80

namespace QM
{
	IODefineMeta(VBEDevice, IO::PCIDevice)

	void VBEDevice::InitialWakeUp(IO::MetaClass *meta)
	{
		if(meta == VBEDevice::GetMetaClass())
		{
			IO::Dictionary *properties = IO::Dictionary::Alloc()->Init();

			IO::String *className = IO::String::Alloc()->InitWithCString("IO::PCIProvider", false);

			properties->SetObjectForKey(className, IO::kServiceProviderMatchKey);
			properties->SetObjectForKey(IO::Number::Alloc()->InitWithUint16(0x1234), IO::kPCIDeviceVendorIDKey);
			properties->SetObjectForKey(IO::Number::Alloc()->InitWithUint16(0x1111), IO::kPCIDeviceDeviceIDKey);

			className->Release();

			RegisterService(meta, properties);
			properties->Release();
		}

		IO::Service::InitialWakeUp(meta);
	}

	VBEDevice *VBEDevice::InitWithProperties(IO::Dictionary *properties)
	{
		if(!IO::PCIDevice::InitWithProperties(properties))
			return nullptr;

		_width = 1024;
		_height = 768;

		_framebufferRegion = nullptr;
		_framebuffer = nullptr;

		return this;
	}

	void VBEDevice::Dealloc()
	{
		if(_framebuffer)
			_framebuffer->Release();

		if(_framebufferRegion)
			_framebufferRegion->Release();

		IO::PCIDevice::Dealloc();
	}


	void VBEDevice::WriteRegister(uint16_t index, uint16_t value)
	{
		outw(VBE_DISPI_IOPORT_INDEX, index);
		outw(VBE_DISPI_IOPORT_DATA, value);
	}

	uint16_t VBEDevice::ReadRegister(uint16_t index)
	{
		outw(VBE_DISPI_IOPORT_INDEX, index);
		return inw(VBE_DISPI_IOPORT_DATA);
	}

	void VBEDevice::SwitchResolution(size_t width, size_t height)
	{
		_width = static_cast<uint16_t>(width);
		_height = static_cast<uint16_t>(height);

		WriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED | VBE_DISPI_GETCAPS);

		WriteRegister(VBE_DISPI_INDEX_XRES, _width);
		WriteRegister(VBE_DISPI_INDEX_YRES, _height);
		WriteRegister(VBE_DISPI_INDEX_BPP, 32);
		WriteRegister(VBE_DISPI_INDEX_VIRT_HEIGHT, 4096);

		WriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

		if(_framebuffer)
		{
			_framebuffer->Release();
			_framebuffer = nullptr;
		}

		_framebuffer = IO::Framebuffer::Alloc()->InitWithMemory(_framebufferRegion->GetAddress(), _width, _height, 4);
	}

	void VBEDevice::Start()
	{
		IO::PCIDevice::Start();

		IO::PCIDeviceResource *framebuffer = GetResourceWithIndex(0);
		if(!framebuffer)
			return;

		_framebufferRegion = IO::MemoryRegion::Alloc()->InitWithPhysical(framebuffer->GetAddress(), VM_PAGE_COUNT(framebuffer->GetSize()));

		// Version check
		uint16_t version = ReadRegister(VBE_DISPI_INDEX_ID);
		if(version < VBE_DISPI_ID0 || version > VBE_DISPI_ID4)
		{
			kprintf("Version is %x\n", version);
			return;
		}

		WriteRegister(VBE_DISPI_INDEX_ID, VBE_DISPI_ID4);

		if(ReadRegister(VBE_DISPI_INDEX_ID) != VBE_DISPI_ID4)
		{
			kprintf("Couldn't enforce VBE version");
			return;
		}

		// Set up
		WriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED | VBE_DISPI_GETCAPS);

		// Get capabilities
		_maxWidth = ReadRegister(VBE_DISPI_INDEX_XRES);
		_maxHeight = ReadRegister(VBE_DISPI_INDEX_YRES);
		_maxDepth = ReadRegister(VBE_DISPI_INDEX_BPP);

		if(_maxWidth < 1024 || _maxHeight < 768)
			return;

		SwitchResolution(1024, 768); // Will automatically enable it again

		_display = IO::Display::Alloc()->InitWithFramebuffer(_framebuffer);
		_display->Publish();
	}
}
