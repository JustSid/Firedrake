//
//  syslog.c
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
#include "syslog.h"
#include "video.h"

#define SYSLOG_BUFFER_SIZE 512

static syslog_level_t __syslog_level = LOG_WARNING;
static vd_color_t __sylog_color_table[] = {
	vd_color_red,			// LOG_ALERT
	vd_color_lightRed, 		// LOG_CRITICAL
	vd_color_brown, 		// LOG_ERROR
	vd_color_yellow, 		// LOG_WARNING
	vd_color_lightGray, 	// LOG_INFO
	vd_color_lightBlue 		// LOG_DEBUG
};

void syslog(syslog_level_t level, const char *format, ...)
{
	if(level > __syslog_level)
		return;
	
	char buffer[SYSLOG_BUFFER_SIZE];

	va_list arguments;
	va_start(arguments, format);
	
	vsnprintf(buffer, SYSLOG_BUFFER_SIZE, format, arguments);

	va_end(arguments);

	vd_printString(buffer, __sylog_color_table[(int)level]);
}

void setLogLevel(syslog_level_t level)
{
	if(level > LOG_DEBUG)
		return;
	
	__syslog_level = level;
}
