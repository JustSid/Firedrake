//
//  string.h
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

#ifndef _STRING_H_
#define _STRING_H_

#include <prefix.h>
#include "stddef.h"

#define isdigit(c) (c >= '0' && c <= '9')
#define isspace(c) (c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r')

__BEGIN_DECLS

void *memset(void *dst, int c, size_t size);
void *memcpy(void *dst, const void *src, size_t size);

char *strcpy(char *dst, const char *src);
size_t strlcpy(char *dst, const char *src, size_t size); // Similar to strncpy, but appends the NULL byte always!
size_t strlen(const char *string);

int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, size_t size);

char *strstr(char *str1, const char *str2);
char *strpbrk(char *str1, const char *str2);
char *strchr(char *str, int character);

__END_DECLS

#endif /* _STRING_H_ */
