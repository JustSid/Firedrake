//
//  time.h
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

#ifndef _TIME_H_
#define _TIME_H_

#include <prefix.h>

#define TIME_FREQUENCY 1000
#define TIME_MILLISECS_PER_TICK 1

typedef struct
{
	uint32_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} date_components_t;

typedef int64_t timestamp_t; // Milliseconds
typedef int32_t unix_time_t; // Seconds

int32_t time_getSeconds(timestamp_t time); // Returns the seconds from a timestamp
int32_t time_getMilliseconds(timestamp_t time); // Returns the milliseconds from a timestamp

timestamp_t time_convertUnix(unix_time_t time);
unix_time_t time_convertTimestamp(timestamp_t time);

timestamp_t time_getTimestamp(); // Returns the milliseconds since boot
unix_time_t time_getUnixTime(); // Returns the current unix timestamp
unix_time_t time_getBootTime(); // Returns the unix timestamp of the boot time

unix_time_t time_create(date_components_t *components);

bool time_init(void *unused); // Assumes that no interrupts are enabled!

#endif /* _TIME_H_ */
