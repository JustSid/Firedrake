//
//  stdio.c
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

#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "stdbool.h"
#include "stdint.h"

extern int _itostr(int32_t i, int base, char *buffer, bool lowerCase);
extern int _itostr64(int64_t i, int base, char *buffer, bool lowerCase);
extern int _uitostr(uint32_t i, int base, char *buffer, bool lowerCase);
extern int _uitostr64(uint64_t i, int base, char *buffer, bool lowerCase);

#define kVsnprintfFlagRightAlign (1 << 0) // '-'
#define kVsnprintfFlagForceSign  (1 << 1) // '+'
#define kVsnprintfZeroPad        (1 << 2) // '0'

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
			
			// Get the length
			long typeLength = 4;

			switch(format[index])
			{
				case 'h':
				{
					typeLength = 2;
					index ++;
					
					if(format[index] == 'h')
					{
						typeLength = 1;
						index ++;
					}
					
					break;
				}
				
				case 'l':
				{
					typeLength = 4;
					index ++;

					if(format[index] == 'l')
					{
						typeLength = 8;
						index ++;
					}

					break;
				}
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
						string = "(null)";
					
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
					char specifier = format[index];

					int  base = (specifier == 'i' || specifier == 'u' || specifier == 'd') ? 10 : 16;
					bool isUint = (specifier== 'p' || specifier == 'u' || specifier == 'x');
					bool lowerCase = (specifier == 'x');
					
					char *prefix = (specifier == 'p') ? "0x" : "\0";
					char string[128];

					if(specifier == 'p')
					{
						void *value = va_arg(arg, void *);
						_uitostr((uint32_t)value, base, string, false);
					}
					else
					{
						switch(typeLength)
						{
							case 1:
							case 2:
							case 4:
							{
								if(isUint)
								{
									uint32_t value = va_arg(arg, uint32_t);
									_uitostr(value, base, string, lowerCase);
								}
								else
								{
									int32_t value = va_arg(arg, int32_t);
									_itostr(value, base, string, lowerCase);
								}

								break;
							}

							case 8:
							{
								if(isUint)
								{
									uint64_t value = va_arg(arg, uint64_t);
									_uitostr64(value, base, string, lowerCase);
								}
								else
								{
									int64_t value = va_arg(arg, int64_t);
									_itostr64(value, base, string, lowerCase);
								}

								break;
							}

							default:
								break;
						}
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

				case 'n':
				{
					int *value = va_arg(arg, int *);
					*value = (int)written;

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
