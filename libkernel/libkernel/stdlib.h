//
//  stdlib.h
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

#ifndef _LIBKERNEL_STDLIB_H_
#define _LIBKERNEL_STDLIB_H_

#include "base.h"
#include "stdint.h"

#define STDLIBBUFFERLENGTH 128

kern_extern int atoi(const char *string);

kern_extern int _itostr(int i, int base, char *buffer, bool lowerCase); // Converts an integer to a string and copies the result into buffer
kern_extern int _uitostr(unsigned int i, int base, char *buffer, bool lowerCase);

kern_extern int _itostr64(int64_t i, int base, char *buffer, bool lowerCase);
kern_extern int _uitostr64(uint64_t i, int base, char *buffer, bool lowerCase);

#endif /* _LIBKERNEL_STDLIB_H_ */
