//
//  kernel.h
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

#ifndef _kernel_h_
#define _kernel_h_

#include <types.h>
#include <ioglue/iostore.h>

const char *kern_nameForAddress(uintptr_t address, io_library_t **outLibrary);
uintptr_t kern_resolveAddress(uintptr_t address);

void kern_printBacktrace(long depth);

typedef enum 
{
	kern_breakOnExecution = 0,
	kern_breakOnWrite = 1,
	//kern_breakOnIO = 2, // According to Wikipedia, there is no processor supporting this!
	kern_breakOnReadWrite = 3
} kern_breakCondition;

typedef enum
{
	kern_watchOneByte = 0,
	kern_watchTwoBytes = 1,
	kern_watchFourBytes = 3,
	kern_watchEightBytes = 2
} kern_watchBytes;

void kern_setWatchpoint(uint8_t reg, bool global, uintptr_t address, kern_breakCondition condition, kern_watchBytes bytes);
void kern_disableWatchpoint(uint8_t reg);

#endif
