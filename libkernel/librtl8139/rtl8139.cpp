//
//  RTL8139.cpp
//  libRTL8139
//
//  Created by Sidney
//  Copyright (c) 2012 by Sidney
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

#include <libkernel/libkernel.h>
#include <libio/libio.h>
#include <libPCI/PCIProvider.h>

#include "RTL8139Controller.h"

void RTL8139_bootstrap(IOThread *)
{
	IOAutoreleasePool *pool = IOAutoreleasePool::alloc()->init();

	// Wait for libPCI to become available
	IOModule *pciModule = IOModule::withName("libPCI.so");
	pciModule->waitForPublish();

	// Make our service available
	IODictionary *properties = IODictionary::withObjectsAndKeys(IONumber::withUInt8(kPCIDeviceClassNetworkController), PCIDevicePropertyClassID,
																IONumber::withUInt16(0x10EC), PCIDevicePropertyVendorID,
																IONumber::withUInt16(0x8139), PCIDevicePropertyDeviceID, 0);

	IOService::registerService(IOSymbol::withName("RTL8139Controller"), IOService::createMatchingDictionary(PCIDeviceFamily, properties));

	// Request a new probe
	pciModule->requestProbe();
	pool->release();
}

bool RTL8139_start(kern_module_t *UNUSED(module))
{
	IOThread *thread = IOThread::alloc()->initWithFunction(RTL8139_bootstrap);
	thread->detach();
	thread->release();

	return true;
}

bool RTL8139_stop(kern_module_t *UNUSED(module))
{
	return true;
}

kern_start_stub(RTL8139_start);
kern_stop_stub(RTL8139_stop);
