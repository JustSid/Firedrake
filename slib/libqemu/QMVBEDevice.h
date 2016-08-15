//
//  QMVBEDevice.h
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

#ifndef _QMVBEDEVICE_H_
#define _QMVBEDEVICE_H_

#include <core/IOMemoryRegion.h>
#include <video/IOFramebuffer.h>
#include <video/IODisplay.h>
#include <IOPCIDevice.h>

namespace QM
{
	class VBEDevice : public IO::PCIDevice
	{
	public:
		static void InitialWakeUp(IO::MetaClass *meta);

		VBEDevice *InitWithProperties(IO::Dictionary *properties) override;
		void Dealloc() override;

		void Start() override;

	private:
		void WriteRegister(uint16_t index, uint16_t value);
		uint16_t ReadRegister(uint16_t index);

		void SwitchResolution(size_t width, size_t height);

		IO::MemoryRegion *_framebufferRegion;
		IO::Framebuffer *_framebuffer;

		IO::Display *_display;

		uint16_t _width;
		uint16_t _height;

		uint16_t _maxWidth;
		uint16_t _maxHeight;
		uint16_t _maxDepth;

		IODeclareMeta(VBEDevice)
	};
}

#endif /* _QMVBEDEVICE_H_ */
