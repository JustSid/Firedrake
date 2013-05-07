//
//  IOLog.cpp
//  libio
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

#include <libkernel/ctype.h>
#include <libkernel/stdlib.h>
#include <libkernel/string.h>

#include "IOLog.h"
#include "IOObject.h"
#include "IOString.h"

#define kVsnprintfFlagRightAlign (1 << 0) // '-'
#define kVsnprintfFlagForceSign  (1 << 1) // '+'
#define kVsnprintfZeroPad		 (1 << 2) // '0'

int __IOvsnprintf(char *buffer, size_t size, const char *format, va_list arg)
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

				case 'i':
				{
					typeLength = 4;
					index ++;

					if(format[index] == 'i')
					{
						typeLength = 8;
						index ++;
					}

					break;
				}

				default:
					break;
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
					char character = format[index];

					int  base = (character == 'i' || character == 'd' || character == 'u') ? 10 : 16;
					bool isUint = (character == 'u' || character == 'x');
					bool lowercase = (character == 'x');
					
					const char *prefix = (character == 'p') ? "0x" : "\0";
					char string[STDLIBBUFFERLENGTH];

					if(character == 'p')
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
									_uitostr(value, base, string, lowercase);
								}
								else
								{
									int32_t value = va_arg(arg, int32_t);
									_itostr(value, base, string, lowercase);
								}

								break;
							}

							case 8:
							{
								if(isUint)
								{
									uint64_t value = va_arg(arg, uint64_t);
									_uitostr64(value, base, string, lowercase);
								}
								else
								{
									int64_t value = va_arg(arg, int64_t);
									_itostr64(value, base, string, lowercase);
								}

								break;
							}
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

				case '@':
				{
					IOObject *object = va_arg(arg, IOObject *);

					const char *printString = (object != 0) ? object->description()->CString() : "(null)";
					while(*printString != '\0')
					{
						if(keepWriting && written < size)
							buffer[written ++] = *printString;

						total ++;
						printString ++;
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

#define IOLOG_BUFFER_SIZE 1024
extern "C" void __io_primitiveLog(const char *message, bool appendNewline);

void IOLog(const char *message, ...)
{
	char buffer[IOLOG_BUFFER_SIZE];

	va_list arguments;
	va_start(arguments, message);
	
	__IOvsnprintf(buffer, IOLOG_BUFFER_SIZE, message, arguments);
	__io_primitiveLog(buffer, true);

	va_end(arguments);
}
