//
//  acpi.h
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

#ifndef _ACPI_H_
#define _ACPI_H_

#include <prefix.h>
#include <kern/kern_return.h>
#include <libc/stdint.h>
#include <libc/stddef.h>

namespace ACPI
{
	struct RSDT;
	struct MADTEntry;

	struct RSDP
	{
		uint64_t signature;
		uint8_t checksum;
		int8_t oemID[6];
		uint8_t revision;
		uint32_t rsdtAddress;

		RSDT *GetRSDT() const;
	} __attribute__((packed));

	struct RSDTHeader
	{
		int8_t signature[4];
		uint32_t length;
		uint8_t revision;
		uint8_t checksum;
		int8_t oemID[6];
		int8_t oemTableID[8];
		uint32_t oemRevision;
		uint32_t creatorID;
		uint32_t creatorRevision;
	} __attribute__((packed));

	struct RSDT : public RSDTHeader
	{
		uint32_t *otherSDT;

		RSDTHeader *FindHeader(const char *signature) const;

		template<class T>
		T *FindEntry(const char *signature) const
		{
			return static_cast<T *>(FindHeader(signature));
		}

		kern_return_t ParseMADT() const;
	} __attribute__((packed));

	struct MADT : public RSDTHeader
	{
		uint32_t interruptControllerAddress;
		uint32_t flags;

		size_t GetEntryCount() const;
		MADTEntry *GetFirstEntry() const;

	} __attribute__((packed));

	struct MADTEntry
	{
		uint8_t type;
		uint8_t length;

		MADTEntry *GetNext() const;
	} __attribute__((packed));


	struct MADTApic : public MADTEntry
	{
		uint8_t acpiID;
		uint8_t apicID;
		uint32_t flags;
	} __attribute__((packed));

	struct MADTIOApic : public MADTEntry
	{
		uint8_t ioapicID;
		uint8_t reserved;
		uint32_t ioapicAddress;
		uint32_t interruptBase;
	} __attribute__((packed));

	struct MADTInterruptSource : public MADTEntry
	{
		uint8_t bus;
		uint8_t source;
		uint32_t globalSystemInterrupt;
		uint16_t flags;
	} __attribute__((packed));

	kern_return_t Init();
}

#endif /* _ACPI_H_ */
