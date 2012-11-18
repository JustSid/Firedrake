//
//  stdio.c
//  libkernel
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

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "ctype.h"

#define kVsnprintfFlagRightAlign (1 << 0) // '-'
#define kVsnprintfFlagForceSign  (1 << 1) // '+'
#define kVsnprintfZeroPad		 (1 << 2) // '0'

int vsnprintf(char *buffer, size_t size, const char *format, va_list arg)
{
	size_t written = 0;
	size_t total = 0;

	uint32_t index = 0;
	bool keepWriting = true;
	
	while(format[index] != '\0')
	{
		if(written >= size)
			keepWriting = false;

		if(format[index] == '%')
		{
			index ++;

			// Fetch flags
			long flags = 0;
			while(1)
			{
				if(format[index] == '0')
				{
					if(flags != 0)
						break;


					flags |= kVsnprintfZeroPad;
					index ++;
					continue;
				}

				if(format[index] == '-')
				{
					flags |= kVsnprintfFlagRightAlign;
					index ++;
					continue;
				}

				if(format[index] == '+')
				{
					flags |= kVsnprintfFlagForceSign;
					index ++;
					continue;
				}

				break;
			}

			// Get the width, if supplied
			long width = 0;
			while(isdigit(format[index]))
			{
				int value = format[index] - '0';
				width = (width * 10) + value;

				index ++;
			}
			

			switch(format[index])
			{
				case 'c':
				{
					if(keepWriting)
					{
						char character = (char)va_arg(arg, int);
						buffer[written ++] = (char)character;
					}

					total ++;
					break;
				}
				
				case 's':
				{
					char *string = va_arg(arg, char *);
					if(!string)
						string = (char *)"(null)";
					
					while(*string != '\0')
					{
						if(keepWriting && written < size)
						{
							buffer[written ++] = *string;
						}

						total ++;
						string ++;
					}
					
					break;
				}
					
				case 'i':
				case 'd':
				case 'p':
				case 'x':
				case 'X':
				case 'u':
				{
					int  base = (format[index] == 'i' || format[index] == 'd') ? 10 : 16;
					bool pointer = (format[index] == 'p');
					bool isUint = (format[index] == 'p' || format[index] == 'u' || format[index] == 'x');
					
					const char *prefix = (pointer) ? "0x" : "\0";
					char string[STDLIBBUFFERLENGTH];
					
					if(isUint)
					{
						unsigned long value = va_arg(arg, unsigned long);
						_uitostr(value, base, string, (format[index] == 'x'));
					}
					else 
					{
						long value = va_arg(arg, long);
						_itostr(value, base, string, (format[index] == 'x'));
					}

				
					
					while(*prefix != '\0')
					{
						if(keepWriting && written < size)
							buffer[written ++] = *prefix;

						total ++;
						prefix ++;
					}

					// Left pad
					if(!(flags & kVsnprintfFlagRightAlign))
					{
						width -=  strlen(string);

						while(width > 0)
						{
							if(keepWriting && written < size)
								buffer[written ++] = (flags & kVsnprintfZeroPad) ? '0' : ' ';

							width --;
							total ++;
						}
					}
					
					// Print the actual string
					char *printString = (char *)string;
					while(*printString != '\0')
					{
						if(keepWriting && written < size)
							buffer[written ++] = *printString;

						total ++;
						printString ++;
					}

					// Right pad
					if((flags & kVsnprintfFlagRightAlign))
					{
						width -=  strlen(string);

						while(width > 0)
						{
							if(keepWriting && written < size)
								buffer[written ++] = ' ';

							total ++;
							width --;
						}
					}
					
					
					break;
				}
					
				case '%':
				{
					if(keepWriting)
						buffer[written ++] = '%';		

					total ++;
					break;
				}
					
				default:
					break;
			}
		}
		else
		{
			if(keepWriting)
				buffer[written ++] = format[index];

			total ++;
		}
		
		index ++;
	}
	
	if(buffer)
	{
		index = (written >= size) ? written - 1 : written;
		buffer[index] = '\0';
	}
	
	return (int)total;
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

void __IOPrimitiveLog(const char *message);

void printf(const char *format, ...)
{
	
	va_list args;
	va_start(args, format);

	char buffer[1024];
	vsnprintf(buffer, 1024, format, args);

	__IOPrimitiveLog(buffer);

	va_end(args);
}
