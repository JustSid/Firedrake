//
//  config.h
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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifndef CONFIG_RELEASE
	#define CONFIG_RELEASE 0
#endif

#ifndef CONFIG_STRICT
	#define CONFIG_STRICT 1
#endif

#ifndef CONFIG_MAX_CPUS
	#define CONFIG_MAX_CPUS 32
#endif

#ifndef CONFIG_MAX_FILES
	#define CONFIG_MAX_FILES 256
#endif

/**
 * Symbolic names for personality and bootloaders
 **/
#define BOOTLOADER_MULTIBOOT 1
#define BOOTLOADER_DASUBOOT  2

#define PERSONALITY_PC 1

/**
 * Personality dependend settings
 **/

#if PERSONALITY == PERSONALITY_PC
	
	/**
	 * x86 PC with BIOS
	 **/

	#define PERSONALITY_HEADER <personality/pc/pcpersonality.h>
	#define PERSONALITY_CLASS Sys::PCPersonality

	#ifndef CONFIG_MAX_IOAPICS
		#define CONFIG_MAX_IOAPICS 8
	#endif

	#ifndef CONFIG_MAX_INTERRUPT_OVERRIDES
		#define CONFIG_MAX_INTERRUPT_OVERRIDES 32
	#endif

#else
	#error "No or unknown personality specified!"
#endif

#endif /* _CONFIG_H_ */
