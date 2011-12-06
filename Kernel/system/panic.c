//
//  panic.c
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

#include <libc/stdio.h>
#include "panic.h"
#include "syslog.h"

void panic(const char *format, ...)
{
	if(!format)
		panic("No reason given."); // Inception!

	va_list args;	
	va_start(args, format);
	
	char buffer[1024];
	vsnprintf(buffer, 1024, format, args);
	
	va_end(args);

	
	syslog(LOG_ALERT, "\nKernel Panic!");
	syslog(LOG_INFO, "\nReason: ");
	syslog(LOG_ERROR, "\"%s\"", buffer);
	syslog(LOG_INFO, "\n\nCPU halted!");
	
	while(1) 
		__asm__ volatile("cli; hlt"); // And this dear kids is how Mr. Kernel died, alone and without any friends.
}
