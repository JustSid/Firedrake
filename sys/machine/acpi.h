//
//  acpi.h
//  Firedrake
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

#ifndef _ACPI_H_
#define _ACPI_H_

#include <prefix.h>
#include <kern/kern_return.h>
#include <libc/stdint.h>
#include <libc/stddef.h>

typedef struct
{
	uint64_t signature;
	uint8_t checksum;
	int8_t oem_id[6];
	uint8_t revision;
	uint32_t rsdt_address;
} __attribute__((packed)) acpi_rsdp_t;

typedef struct
{
	int8_t signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	int8_t oem_id[6];
	int8_t oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
} __attribute__((packed)) acpi_rsdt_header_t;

typedef struct
{
	acpi_rsdt_header_t header;
	uint32_t *other_sdt;
} __attribute__((packed)) acpi_rsdt_t;

typedef struct
{
	acpi_rsdt_header_t header;
	uint32_t interrupt_controller_address;
	uint32_t flags;
} __attribute__((packed)) acpi_madt_t;

typedef struct
{
	uint8_t type;
	uint8_t length;
} __attribute__((packed)) acpi_madt_entry_t;

typedef struct
{
	acpi_madt_entry_t header;
	uint8_t acpi_processor_id;
	uint8_t apic_id;
	uint32_t flags;
} __attribute__((packed)) acpi_madt_apic_t;

typedef struct
{
	acpi_madt_entry_t header;
	uint8_t ioapic_id;
	uint8_t reserved;
	uint32_t ioapic_address;
	uint32_t interrupt_base;
} __attribute__((packed)) acpi_madt_ioapic_t;

typedef struct
{
	acpi_madt_entry_t header;
	uint8_t bus;
	uint8_t source;
	uint32_t global_system_interrupt;
	uint16_t flags;
} __attribute__((packed)) acpi_madt_interrupt_source_override_t;

kern_return_t acpi_init();

#endif /* _ACPI_H_ */
