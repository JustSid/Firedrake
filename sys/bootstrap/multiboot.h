//
//  multiboot.h
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

#ifndef _MULTIBOOT_H_
#define _MULTIBOOT_H_

#include <prefix.h>
#include <libc/stdint.h>

// http://www.gnu.org/software/grub/manual/multiboot/multiboot.html

#define kMultibootFlagMemory        (1 << 0)
#define kMultibootFlagBootDevice    (1 << 1)
#define kMultibootFlagCommandLine   (1 << 2)
#define kMultibootFlagModules       (1 << 3)

#define kMultibootFlagMmap          (1 << 6)
#define kMultibootFlagDrives        (1 << 7)
#define kMultibootFlagConfigTable   (1 << 8)
#define kMultibootFlagBootLoader    (1 << 9)
#define kMultibootFlagAPM           (1 << 10)

struct multiboot_s
{
	uint32_t flags;

	// kMultibootFlagMemory
	uint32_t mem_lower;
	uint32_t mem_upper;
	
	// kMultibootFlagBootDevice
	uint32_t bootdevice;

	// kMultibootFlagCommandLine
	void *cmdline;
	
	// kMultibootFlagModules
	uint32_t mods_count;
	void *mods_addr;
	
	uint32_t syms[4];
	
	// kMultibootFlagMmap
	uint32_t mmap_length;
	void *mmap_addr;

	// kMultibootFlagDrives
	uint32_t drives_length;
	void *drives_addr;

	// kMultibootFlagConfigTable
	void *config_table;

	// kMultibootFlagBootLoader
	void *boot_loader_name;

	// kMultibootFlagAPM
	void *apm_table;
} __attribute__((packed));

struct multiboot_module_s
{
	void *start;
	void *end;
	void *name;
	uint32_t reserved;
} __attribute__((packed));

struct multiboot_drive_s
{
	uint32_t size;
	uint32_t drive_number;
	char drive_mode;
	char drive_cylinders;
	char drive_heads;
	char drive_sectors;
	uint16_t drive_port0; // First port, there might be more! Last port in the list is 0
} __attribute__((packed));

struct multiboot_mmap_s
{
	uint32_t size;
	uint64_t base;
	uint64_t length;
	uint32_t type;
} __attribute__((packed));

typedef struct multiboot_s         multiboot_t;
typedef struct multiboot_module_s  multiboot_module_t;
typedef struct multiboot_drive_s   multiboot_drive_t;
typedef struct multiboot_mmap_s    multiboot_mmap_t;

extern multiboot_t *bootinfo;

#endif
