//
//  PCIClass.h
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

#ifndef _PCICLASS_H_
#define _PCICLASS_H_

typedef enum
{
	kPCIDeviceClassLegacy = 0x0,
	kPCIDeviceClassMassStorageController = 0x1,
	kPCIDeviceClassNetworkController = 0x2,
	kPCIDeviceClassDisplayController = 0x3,
	kPCIDeviceClassMultimediaController = 0x4,
	kPCIDeviceClassMemoryController = 0x5,
	kPCIDeviceClassBridgeDevice = 0x6,
	kPCIDeviceClassSimpleCommunicationController = 0x7,
	kPCIDeviceClassBaseSystemPeripherals = 0x8,
	kPCIDeviceClassInputDevice = 0x9,
	kPCIDeviceClassDockingStation = 0xA,
	kPCIDeviceClassProcssor = 0xB,
	kPCIDeviceClassSerialBusController = 0xC,
	kPCIDeviceClassWirelessController = 0xD,
	kPCIDeviceClassIntelligentIOController = 0xE,
	kPCIDeviceClassSatelliteCommunicationController = 0xF,
	kPCIDeviceClassCryptoController = 0x10,
	kPCIDeviceClassSignalProcessingController = 0x11,
	kPCIDeviceClassUndefined = 0xFF
} PCIDeviceClass;

#endif /* _PCICLASS_H_ */
