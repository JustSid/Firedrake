//
//  acpi.cpp
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

#include <kern/kprintf.h>
#include <libc/string.h>
#include <machine/interrupts/apic.h>
#include "acpi.h"

namespace ACPI
{
	constexpr uint64_t RSDPSignature = UINT64_C(0x2052545020445352);
	static RSDP *_rsdp = nullptr;

	RSDT *RSDP::GetRSDT() const
	{
		static bool resolvedOtherSDT = false;

		RSDT *rsdt = reinterpret_cast<RSDT *>(rsdtAddress);

		if(!resolvedOtherSDT)
		{
			rsdt->otherSDT = reinterpret_cast<uint32_t *>(&rsdt->otherSDT);
			resolvedOtherSDT = true;
		}

		return rsdt;
	}

	RSDTHeader *RSDT::FindHeader(const char *signature) const
	{
		size_t entries = (length - sizeof(RSDTHeader)) / 4;

		for(size_t i = 0; i < entries; i ++)
		{
			RSDTHeader *header = reinterpret_cast<RSDTHeader *>(otherSDT[i]);

			if(strncmp(signature, reinterpret_cast<char *>(header->signature), 4) == 0)
				return header;
		}

		return nullptr;
	}

	KernReturn<void> RSDT::ParseMADT() const
	{
		MADT *madt = FindEntry<MADT>("APIC");

		if(madt)
		{
			MADTEntry *entry = madt->GetFirstEntry();
			size_t entries = madt->GetEntryCount();

			for(size_t i = 0; i < entries; i ++)
			{
				switch(entry->type)
				{
					case 0:
					{
						MADTApic *apic = static_cast<MADTApic *>(entry);
						uint8_t enabled = (apic->flags & 0x1);

						if(enabled)
							Sys::CPU::RegisterCPU(apic->apicID, 0);

						break;
					}

					case 1:
					{
						MADTIOApic *ioapic = static_cast<MADTIOApic *>(entry);
						Sys::IOAPIC *result = Sys::IOAPIC::RegisterIOAPIC(ioapic->ioapicID, ioapic->ioapicAddress, ioapic->interruptBase);

						if(!result)
							return Error(KERN_RESOURCES_MISSING);

						break;
					}

					case 2:
					{
						MADTInterruptSource *override = static_cast<MADTInterruptSource *>(entry);
						Sys::IOAPIC::InterruptOverride *result = Sys::IOAPIC::RegisterInterruptOverride(override->source, override->flags, override->globalSystemInterrupt);

						if(!result)
							return Error(KERN_RESOURCES_MISSING);

						break;
					}
				}

				entry = entry->GetNext();
			}

			return ErrorNone;
		}

		return Error(KERN_FAILURE);
	}


	size_t MADT::GetEntryCount() const
	{
		size_t entries = (length - sizeof(MADT)) / 4;
		return entries;
	}

	MADTEntry *MADT::GetFirstEntry() const
	{
		return reinterpret_cast<MADTEntry *>(const_cast<MADT *>(this + 1));
	}

	MADTEntry *MADTEntry::GetNext() const
	{
		uint8_t *temp = reinterpret_cast<uint8_t *>(const_cast<MADTEntry *>(this));
		return reinterpret_cast<MADTEntry *>(temp + length);
	}





	RSDP *FindRSDPInRange(uintptr_t start, size_t size)
	{
		uint16_t *pointer  = reinterpret_cast<uint16_t *>(start);
		uint16_t *rangeEnd = reinterpret_cast<uint16_t *>(start + size);

		while(pointer != rangeEnd)
		{
			RSDP *rsdp = reinterpret_cast<RSDP *>(pointer);
			if(rsdp->signature == RSDPSignature)
			{
				uint8_t *temp = reinterpret_cast<uint8_t *>(rsdp);
				uint8_t *end  = reinterpret_cast<uint8_t *>(rsdp + 1);

				// Calculate the checksum
				uint8_t checksum = 0;
				while(temp != end)
					checksum += *temp ++;

				if(checksum == 0)
					return rsdp;
			}

			pointer ++;
		}

		return nullptr;
	}

	RSDP *FindRSDP()
	{
		if((_rsdp = FindRSDPInRange(0xe0000, 0x20000)))
			return _rsdp;

		return nullptr;
	}

	KernReturn<void> Init()
	{
		if(!FindRSDP())
		{
			kprintf("Couldn't find ACPI table");
			return Error(KERN_FAILURE);
		}

		KernReturn<void> result;
		RSDT *rsdt = _rsdp->GetRSDT();

		if((result = rsdt->ParseMADT()).IsValid() == false)
		{
			kprintf("Failed to parse MADT");
			return result;
		}

		return ErrorNone;
	}
}
