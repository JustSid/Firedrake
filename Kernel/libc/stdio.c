//
//  stdio.c
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

#include <system/syscall.h>
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

int vsnprintf(char *buffer, size_t size, const char *format, va_list arg)
{
	size_t written = 0;
	uint32_t index = 0;
	
	while(format[index] != '\0' && written < size)
	{
		if(format[index] == '%')
		{
			index ++;
			
			switch(format[index])
			{
				case 'c':
				{
					char character = va_arg(arg, int);
					buffer[written ++] = (char)character;
					break;
				}
				
				case 's':
				{
					char *string = va_arg(arg, char *);
					if(!string)
						string = "(null)";
					
					while(*string != '\0' && written < size)
					{
						buffer[written ++] = *string++;
					}
					
					break;
				}
					
				case 'i':
				case 'd':
				case 'p':
				case 'x':
				case 'u':
				{
					int base = (format[index] == 'i' || format[index] == 'd') ? 10 : 16;
					bool pointer = (format[index] == 'p');
					bool isUint = (format[index] == 'p' || format[index] == 'u' || format[index] == 'x');
					
					char *prefix = (pointer) ? "0x" : "\0";
					char string[STDLIBBUFFERLENGTH];
					
					if(isUint)
					{
						unsigned long value = va_arg(arg, unsigned long);
						_uitostr(value, base, string);
					}
					else 
					{
						long value = va_arg(arg, long);
						_itostr(value, base, string);
					}
					
					
					for(int i=0; i<2; i++)
					{
						char *printStr = (i == 0) ? prefix : string;
						while(*printStr != '\0' && written < size)
						{
							buffer[written ++] = *printStr++;
						}
					}
					
					break;
				}
					
				case '%':
				{
					buffer[written++] = '%';		
					break;
				}
					
				default:
				{
					buffer[written++] = '%';	
					buffer[written++] = format[index];
					break;
				}
			}
		}
		else
		{
			buffer[written++] = format[index];
		}
		
		index ++;
	}
	
	if(written >= size)
	{
		buffer[written - 1] = '\0';	
	}
	else
	{
		buffer[written] = '\0';
	}
	
	return (int)written;
}

int vsprintf(char *buffer, const char *format, va_list arg)
{
	return vsnprintf(buffer, UINT32_MAX, format, arg);
}

int snprintf(char *dst, size_t size, const char *format, ...)
{
	va_list param;
	va_start(param, format);
	
	int written = vsnprintf(dst, size, format, param);
	va_end(param);
	
	return written;
}

int sprintf(char *dst, const char *format, ...)
{
	va_list param;
	va_start(param, format);
	
	int written = vsprintf(dst, format, param);
	va_end(param);
	
	return written;
}



void sys_printf(const char *format, ...)
{
	va_list param;
	va_start(param, format);
	
	char buffer[1024];
	vsnprintf(buffer, 1024, format, param);
	sys_printf(buffer);
	
	va_end(param);
	
}
