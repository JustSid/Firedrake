//
//  time.h
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

#ifndef _TIME_H_
#define _TIME_H_

#include <types.h>

#define TIME_FREQUENCY 1000
#define TIME_MILLISECS_PER_TICK 1

typedef uint32_t time_t;
typedef uint64_t timestamp_t; // Precise time

time_t time_getSeconds();
time_t time_getMilliSeconds();
timestamp_t time_getTimestamp();
timestamp_t time_getTimestampSinceBoot();

timestamp_t timestamp_getDifference(timestamp_t timestamp1, timestamp_t timestamp2);

static inline uint32_t timestamp_getSeconds(timestamp_t timestamp)
{
	uint32_t seconds = (uint32_t)(timestamp >> 32);
	return seconds;
}

static inline uint32_t timestamp_getMilliseconds(timestamp_t timestamp)
{
	uint32_t millisecs = (uint32_t)(timestamp & 0xFFFFFFFF);
	return millisecs;
}

time_t time_create(uint32_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

bool time_init(void *unused); // Assumes that no interrupts are enabled!

#endif /* _TIME_H_ */
