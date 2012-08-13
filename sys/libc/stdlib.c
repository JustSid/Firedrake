//
//  stdlib.c
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

#include "stdlib.h"
#include "string.h"

int atoi(const char *string)
{
	while(isspace(*string))
		string ++; // Skip any leading whitespace


	int result = 0;
	bool negative = (*string == '-');
	bool positive = (*string == '+');

	if(negative || positive)
		string ++; // Skip the '-' or '+' sign

	while(*string != '\0' && isdigit(*string))
	{
		int value = *string - '0';
		int intermediate = (result * 10) + value;

		if(intermediate < result)
			break; // Overflow!

		result = intermediate;

		string ++;
	}	

	// FIXME: Underflows are still possible, in rare cases...
	return negative ? -result : result;
}

double atof(const char *string)
{
	while(isspace(*string))
		string ++; // Skip any leading whitespace

	double result = 0.0;
	bool negative = (*string == '-');
	bool positive = (*string == '+');

	int  exponent = 0;
	char character;

	if(negative || positive)
		string ++; // Skip the '-' or '+' sign

	while((character = *string++) != '\0' && isdigit(character)) 
	{
		double value = (double)character - '0';
		result = (result * 10.0) + value;
   	}

	if(character == '.')
	{
   		string ++;

   		while((character = *string++) != '\0' && isdigit(character)) 
		{
			double value = (double)character - '0';
			result = (result * 10.0) + value;

			exponent --;
	   }
	}

	if(character == 'e' || character == 'E')
	{
		// TODO: Could be made faster by using either an inlined atoi version or just reimplementing what we need from atoi!
		exponent += atoi(string);
	}

   	// Process the exponent
   	while(exponent > 0)
   	{
   		result *= 10.0;
   		exponent --;
   	}
   	while(exponent < 0)
   	{
   		result *= 0.1;
   		exponent ++;
   	}

	return negative ? -result : result;
}


int _itostr(int i, int base, char *buffer, bool lowerCase)
{
	if(i == 0)
	{
		strlcpy(buffer, "0", 1);
		return 1;
	}
	

	int length = 0;
	bool negative = (i < 0);
	
	char _buffer[STDLIBBUFFERLENGTH];
	char *temp = _buffer + (STDLIBBUFFERLENGTH - 1);
	const char *digits = (lowerCase) ? "0123456789abcdefghijklmnopqrstuvwxyz" : "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	
	if(negative)
		i = -i;
	

	while(i > 0)
	{
		*--temp = digits[i % base];

		i /= base;
		length ++;
	}
	
	if(negative)
	{
		*--temp = '-';
		length ++;
	}
	
	strlcpy(buffer, (const char *)temp, (size_t)length);
	return length;
}

int _uitostr(unsigned int i, int base, char *buffer, bool lowerCase)
{
	if(i == 0)
	{
		strlcpy(buffer, "0", 1);
		return 1;
	}
	

	int length = 0;
	char _buffer[STDLIBBUFFERLENGTH];
	char *temp = _buffer + (STDLIBBUFFERLENGTH - 1);
	const char *digits = (lowerCase) ? "0123456789abcdefghijklmnopqrstuvwxyz" : "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	while(i > 0)
	{
		*--temp = digits[i % base];

		i /= base;
		length ++;
	}

	strlcpy(buffer, (const char *)temp, (size_t)length);
	return length;
}

