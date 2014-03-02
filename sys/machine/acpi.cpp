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

#define ACPI_RSDP_SIGNATURE UINT64_C(0x2052545020445352)

acpi_rsdp_t *_acpi_rsdp = nullptr;

acpi_rsdp_t *acpi_find_rsdp_in_range(uintptr_t start, size_t size)
{
	uint16_t *ptr = reinterpret_cast<uint16_t *>(start);

	while(reinterpret_cast<uintptr_t>(ptr) < start + size)
	{
		acpi_rsdp_t *rsdp = reinterpret_cast<acpi_rsdp_t *>(ptr);
		if(rsdp->signature == ACPI_RSDP_SIGNATURE)
		{
			uint8_t *temp = reinterpret_cast<uint8_t *>(rsdp);
			uint8_t *end  = reinterpret_cast<uint8_t *>(rsdp + 1);

			uint8_t checksum = 0;
			while(temp != end)
				checksum += *temp ++;

			if(checksum == 0)
				return rsdp;
		}

		ptr ++;
	}

	return nullptr;
}

acpi_rsdp_t *acpi_find_rsdp()
{
	acpi_rsdp_t *rsdp;

	if((rsdp = acpi_find_rsdp_in_range(0xe0000, 0x20000)))
		return rsdp;

	return nullptr;
}

acpi_rsdt_t *acpi_get_rsdt()
{
	acpi_rsdt_t *rsdt = reinterpret_cast<acpi_rsdt_t *>(_acpi_rsdp->rsdt_address);
	rsdt->other_sdt = reinterpret_cast<uint32_t *>(&rsdt->other_sdt);

	return rsdt;
}

acpi_rsdt_header_t *acpi_find_header(const char *signature)
{
	acpi_rsdt_t *rsdt = acpi_get_rsdt();
	size_t entries = (rsdt->length - sizeof(acpi_rsdt_header_t)) / 4;

	for(size_t i = 0; i < entries; i ++)
	{
		acpi_rsdt_header_t *header = reinterpret_cast<acpi_rsdt_header_t *>(rsdt->other_sdt[i]);

		if(strncmp(signature, reinterpret_cast<char *>(header->signature), 4) == 0)
			return header;
	}

	return nullptr;
}

kern_return_t acpi_parse_madt()
{
	bool hasBootstrapCPU = false;
	acpi_madt_t *madt = reinterpret_cast<acpi_madt_t *>(acpi_find_header("APIC"));
	if(madt)
	{
		size_t entries = (madt->length - sizeof(acpi_madt_t)) / 4;
		uint8_t *temp = reinterpret_cast<uint8_t *>(madt + 1);

		for(size_t i = 0; i < entries; i ++)
		{
			acpi_madt_entry_t *entry = reinterpret_cast<acpi_madt_entry_t *>(temp);

			switch(entry->type)
			{
				case 0:
				{
					acpi_madt_apic_t *local_apic = reinterpret_cast<acpi_madt_apic_t *>(entry);
					uint8_t enabled = local_apic->flags & 0x1;

					if(enabled)
					{
						cpu_t cpu;
						cpu.apic_id = local_apic->apic_id;
						cpu.flags   = 0;

						if(!hasBootstrapCPU)
						{
							cpu.flags = (CPU_FLAG_BOOTSTRAP | CPU_FLAG_RUNNING);
							hasBootstrapCPU = true;
						}

						cpu_add_cpu(&cpu);
					}
					break;
				}

				case 1:
				{
					acpi_madt_ioapic_t *ioapic = reinterpret_cast<acpi_madt_ioapic_t *>(entry);
					ir::ioapic_t ioapic_entry;

					ioapic_entry.id = ioapic->ioapic_id;
					ioapic_entry.address = ioapic->ioapic_address;
					ioapic_entry.interrupt_base = ioapic->interrupt_base;

					kern_return_t result = apic_add_ioapic(&ioapic_entry);
					if(result != KERN_SUCCESS)
						return result;

					break;
				}

				case 2:
				{
					acpi_madt_interrupt_source_override_t *isos = reinterpret_cast<acpi_madt_interrupt_source_override_t *>(entry);

					ir::ioapic_interrupt_override_t override;
					override.source = isos->source;
					override.system_interrupt = isos->global_system_interrupt;
					override.flags = isos->flags;

					apic_add_interrupt_override(&override);
					break;
				}
			}

			temp += entry->length;
		}

		return KERN_SUCCESS;
	}

	return KERN_FAILURE;
}


kern_return_t acpi_init()
{
	_acpi_rsdp = acpi_find_rsdp();

	if(_acpi_rsdp)
		return acpi_parse_madt();

	kprintf("couldn't find ACPI table");
	return KERN_FAILURE;
}
