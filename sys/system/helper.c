//
//  helper.c
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2012 by Sidney Just
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

#include <libc/string.h>
#include <libc/stdlib.h>
#include "helper.h"
#include "syslog.h"

#define SYS_UNITTABLE_MAX 6

const char *sys_unitTable[SYS_UNITTABLE_MAX] =
{
	"b",
	"KB",
	"MB",
	"GB",
	"PB",
	"??"
};

const char *sys_unitForSize(size_t size, size_t *result)
{
	uint32_t index = 0;
	while(size > 1024)
	{
		size /= 1024;
		index ++;
	}
	
	if(index >= SYS_UNITTABLE_MAX)
		index = SYS_UNITTABLE_MAX - 1;
	
	if(result)
		*result = size;
	
	return sys_unitTable[index];
}


const char *sys_fileWithoutPath(const char *path)
{
	const char *ptr = (path + strlen(path)) - 1;
	while(ptr != path)
	{
		if(*ptr == '/')
			return ptr + 1;

		ptr --;
	}

	return path;
}

bool isCPPName(const char *name)
{
	return (strstr((char *)name, "_Z") == name);
}

const char *demangleNext(const char *name, char *buffer, char **outbuffer)
{
	if(isdigit(*name))
	{
		int length = atoi(name);
		
		while(isdigit(*name))
			name ++;
		
		for(int i=0; i<length; i++)
			*buffer++ = *name++;

		if(outbuffer)
			*outbuffer = buffer;
	}
	
	return name;
}

void demangleCPPName(const char *name, char *buffer)
{
	name += 2;
	switch(*name)
	{
		case 'N':
		{
			name ++;			
			while(!isdigit(*name))
				name ++;
			
			bool first = true;
			while(isdigit(*name))
			{
				if(!first)
				{
					*buffer++ = ':';
					*buffer++ = ':';
				}
					
				name = demangleNext(name, buffer, &buffer);
				first = false;
			}
			
			break;
		}
		
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		{
			bool first = true;
			while(isdigit(*name))
			{
				if(!first)
				{
					*buffer++ = ':';
					*buffer++ = ':';
				}
					
				name = demangleNext(name, buffer, &buffer);
				first = false;
			}
			
			break;
		}
	}

	*buffer++ = '\0';
}

void sys_dumpgrub()
{
	if(bootinfo->flags & kMultibootFlagBootLoader)
	{
		dbg("Boot loader: \16\27%s\n", bootinfo->boot_loader_name);
	}

	dbg("Command line: \16\27%s\n", bootinfo->cmdline);

	if(bootinfo->flags & kMultibootFlagModules)
	{
		struct multiboot_module_s *module = (struct multiboot_module_s *)bootinfo->mods_addr;

		dbg("Modules: \n");
		for(uint32_t i=0; i<bootinfo->mods_count; i++, module++)
		{
			dbg("  \16\27%s\n", module->name);
		}
	}

	if(bootinfo->flags & kMultibootFlagDrives)
	{
		uint8_t *drivesPtr = bootinfo->drives_addr;
		uint8_t *drivesEnd = drivesPtr + bootinfo->drives_length;

		dbg("Drives: \n");
		while(drivesPtr < drivesEnd)
		{
			struct multiboot_drive_s *drive = (struct multiboot_drive_s *)drivesPtr;

			dbg("  \16\27#%i, %i %i %i (cylinders, heads, sectores)\n", drive->drive_number, (int)drive->drive_cylinders, (int)drive->drive_heads, (int)drive->drive_sectors);

			drivesPtr += drive->size;
		}
	}
}


bool sys_checkCommandline(const char *option, char *buffer)
{
	if(bootinfo->flags & kMultibootFlagCommandLine)
	{
		char *commandLine = bootinfo->cmdline;
		char *found = strstr(commandLine, option);

		if(found)
		{
			bool inQuote = false;

			if(buffer)
			{
				found += strlen(option);
				while(*found)
				{
					switch(*found)
					{
						case '-':
							if(!inQuote)
								return true;

							break;

						case '"':
							inQuote = !inQuote;
							if(!inQuote)
								return true;

							break;

						case ' ':
							if(inQuote)
								*buffer++ = ' ';

							break;

						default:
							*buffer++ = *found;
							break;
					}

					found ++;
				}
			}

			return true;
		}
	}

	return false;
}

struct multiboot_module_s *sys_multibootModuleWithName(const char *name)
{
	if(bootinfo->flags & kMultibootFlagModules)
	{
		struct multiboot_module_s *modules = (struct multiboot_module_s *)bootinfo->mods_addr;
		for(uint32_t i=0; i<bootinfo->mods_count; i++)
		{
			struct multiboot_module_s *module = &modules[i];

			if(strcmp(module->name, name) == 0)
				return module;
		}
	}

	return NULL;
}
