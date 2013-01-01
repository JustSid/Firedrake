//
//  time.c
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

#include <scheduler/scheduler.h>
#include <interrupts/interrupts.h>
#include <libc/bsd/quad.h>
#include "time.h"
#include "port.h"
#include "cmos.h"
#include "syslog.h"

static bool time_gotBootTime = false;
static unix_time_t time_unixBoot = 0; // UNIX timestamp of the boot time
static timestamp_t time_current = 0; // Milliseconds since boot

// Time getter functions
int32_t time_getSeconds(timestamp_t time)
{
	int32_t seconds = (int32_t)__udivdi3(time, 1000);
	return seconds;
}

int32_t time_getMilliseconds(timestamp_t time)
{
	int32_t milliseconds = (int32_t)__umoddi3(time, 1000);
	return milliseconds;
}

timestamp_t time_convertUnix(unix_time_t time)
{
	timestamp_t timestamp = (timestamp_t)time;
	timestamp *= 1000;
	return timestamp;
}

unix_time_t time_convertTimestamp(timestamp_t time)
{
	return (unix_time_t)time_getSeconds(time);
}

timestamp_t time_getTimestamp()
{
	return time_current;
}

unix_time_t time_getUnixTime()
{
	return time_unixBoot + time_getSeconds(time_current);
}

unix_time_t time_getBootTime()
{
	return time_unixBoot;
}

// Time calculation

const uint8_t time_daysOfMonth[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const uint16_t time_daysBeforeMonth[16] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365, 0, 0};

bool time_isLeapYear(uint32_t year)
{
	year = (year + 1) % 400; // correct to nearest multiple of 400 years	
	return ((year & 3) == 0 && year != 100 && year != 200 && year != 300);
}

uint16_t time_daysOfYear(uint32_t year)
{
	return time_isLeapYear(year) ? 366 : 365;
}

unix_time_t time_create(date_components_t *components)
{
	// Convert into UNIX timestamp
	unix_time_t interval = components->second;

	interval += (components->minute * 60);
	interval += (components->hour * 3600);
	interval += ((components->day - 1) * 24 * 3600);
	interval += ((time_daysBeforeMonth[components->month - 1] + time_daysOfMonth[components->month - 1]) * 24 * 3600);

	uint32_t year = components->year;

	while(year > 1970)
	{
		interval += (time_daysOfYear(year) * 24 * 3600);
		year --;
	}

	return interval;
}

// Initialization

void time_readCMOS()
{
	// Read the time from the CMOS
	date_components_t components;

	components.year  = (uint32_t)cmos_readData(CMOS_REGISTER_YEAR) + 2000;
	components.month = (uint8_t)cmos_readData(CMOS_REGISTER_MONTH);
	components.day   = (uint8_t)cmos_readData(CMOS_REGISTER_DMONTH);

	components.hour   = (uint8_t)cmos_readData(CMOS_REGISTER_HOUR);
	components.minute = (uint8_t)cmos_readData(CMOS_REGISTER_MINUTE);
	components.second = (uint8_t)cmos_readData(CMOS_REGISTER_SECOND);

	
	time_unixBoot = time_create(&components);
}

void time_waitForBootTime()
{
	while(!time_gotBootTime)
		__asm__ volatile ("hlt;");
}

uint32_t time_tick(uint32_t esp)
{
	// Update the global time
	time_current ++;
	process_getCurrentProcess()->usedTime ++;

	// Call the scheduler
	esp = sd_schedule(esp);
	return esp;
}

uint32_t time_prepare(uint32_t esp)
{
	static unix_time_t temp = 0;

	if(temp != time_unixBoot || temp == 0)
	{
		temp = time_unixBoot;
		time_readCMOS();

		return esp;
	}

	time_gotBootTime = true;
	ir_setInterruptHandler(time_tick, 0x20);

	return esp;
}

void time_setPITFrequency()
{
	int divisor = 1193180 / TIME_FREQUENCY;

	outb(0x43, 0x36);
	outb(0x40, divisor & 0xFF);
	outb(0x40, divisor >> 8);
}

bool time_init(void *UNUSED(unused))
{
	time_setPITFrequency();
	cmos_writeRTCFlags(CMOS_RTC_FLAG_24HOUR | CMOS_RTC_FLAG_BINARY);

	ir_setInterruptHandler(time_prepare, 0x20);

	dbg("clock speed pinned at %iHz", TIME_FREQUENCY);
	return true;
}
