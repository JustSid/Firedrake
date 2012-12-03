//
//  time.h
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

#ifndef _LIBKERNEL_TIME_H_
#define _LIBKERNEL_TIME_H_

#include "base.h"
#include "stdint.h"

typedef uint32_t unix_time_t;
typedef uint64_t timestamp_t;

kern_extern int32_t time_getSeconds(timestamp_t time);
kern_extern int32_t time_getMilliseconds(timestamp_t time);

kern_extern timestamp_t time_convertUnix(unix_time_t time);
kern_extern unix_time_t time_convertTimestamp(timestamp_t time);

kern_extern timestamp_t time_getTimestamp();
kern_extern unix_time_t time_getUnixTime(); 
kern_extern unix_time_t time_getBootTime();

#endif /* _LIBKERNEL_TIME_H_ */
