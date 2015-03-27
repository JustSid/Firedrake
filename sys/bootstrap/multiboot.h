//
//  multiboot.h
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2014 by Sidney Just
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

#ifndef _MULTIBOOT_H_
#define _MULTIBOOT_H_

#include <prefix.h>
#include <libc/stdint.h>
#include <libcpp/bitfield.h>

#if BOOTLOADER == BOOTLOADER_MULTIBOOT

// http://www.gnu.org/software/grub/manual/multiboot/multiboot.html

namespace Sys
{
	struct MultibootModule
	{
		void *start;
		void *end;
		void *name;
		uint32_t reserved;

		MultibootModule *GetNext() const { return const_cast<MultibootModule *>(this + 1); }
	} __attribute__((packed));

	struct MultibootMmap
	{
		uint32_t size;
		uint64_t base;
		uint64_t length;
		uint32_t type;

		MultibootMmap *GetNext() const { return reinterpret_cast<MultibootMmap *>(const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(this) + size + 4)); }
		bool IsAvailable() const { return type == 1; }
	} __attribute__((packed));

	struct MultibootDrive
	{
		uint32_t size;
		uint32_t drive_number;
		char drive_mode;
		char drive_cylinders;
		char drive_heads;
		char drive_sectors;
		uint16_t drive_port0; // First port, there might be more! Last port in the list is 0
	} __attribute__((packed));



	struct MultibootHeader
	{
		CPP_BITFIELD(Flags, uint32_t,
			Memory      = (1 << 0),
			BootDevice  = (1 << 1),
			CommandLine = (1 << 2),
			Modules     = (1 << 3),
			Mmap        = (1 << 6),
			Drives      = (1 << 7),
			ConfigTable = (1 << 8),
			BootLoader  = (1 << 9),
			APM         = (1 << 10)
		);

		Flags flags;

		uint32_t memLower;
		uint32_t memUpper;

		uint32_t bootDevice;

		void *commandLine;

		uint32_t modulesCount;
		MultibootModule *modules;

		uint32_t syms[4];

		uint32_t mmapLength;
		MultibootMmap *mmap;

		uint32_t drivesLength;
		MultibootDrive *drives;

		void *configTable;
		void *bootLoaderName;
		void *apmTable;


		uint32_t GetMmapCount() const   { return mmapLength / sizeof(MultibootMmap); }
		uint32_t GetDrivesCount() const { return drivesLength / sizeof(MultibootDrive); }

	};

	static_assert(sizeof(MultibootHeader) == 72, "MultibootHeader must be 72 bytes exactly!");

	extern MultibootHeader *bootInfo;
}
#endif /* BOOTLOADER == BOOTLOADER_MULTIBOOT */
#endif /* _MULTIBOOT_H_ */
