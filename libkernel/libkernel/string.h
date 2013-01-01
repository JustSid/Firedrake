//
//  string.h
//  libkernel
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

#ifndef _LIBKERNEL_STRING_H_
#define _LIBKERNEL_STRING_H_

#include "base.h"
#include "stdint.h"

kern_extern void *memset(void *dst, int c, size_t size);
kern_extern void *memcpy(void *dst, const void *src, size_t size);

kern_extern char *strcpy(char *dst, const char *src);
kern_extern size_t strlcpy(char *dst, const char *src, size_t size);
kern_extern size_t strlen(const char *string);

kern_extern int strcmp(const char *str1, const char *str2);
kern_extern int strncmp(const char *str1, const char *str2, size_t size);

kern_extern char *strstr(char *str1, const char *str2);
kern_extern char *strpbrk(char *str1, const char *str2);

#endif /* _LIBKERNEL_STRING_H_ */
