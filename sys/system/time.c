//
//  time.c
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

#include <scheduler/scheduler.h>
#include <interrupts/interrupts.h>
#include "time.h"
#include "port.h"
#include "cmos.h"
#include "syslog.h"

// Holds the time since boot
static time_t time_globalSecs = 0;
static time_t time_globalMilliSecs = 0;

static bool time_gotBootTime = false;
static timestamp_t time_bootTime = 0; // Holds the timestamp of the boot process

// Time getter functions
time_t time_getSeconds()
{
	return time_globalSecs;
}

time_t time_getMilliSeconds()
{
	return time_globalMilliSecs;
}

timestamp_t time_getTimestamp()
{
	timestamp_t timestamp = ((timestamp_t)time_globalSecs << 32) | time_globalMilliSecs;
	return timestamp + time_bootTime;
}

timestamp_t time_getTimestampSinceBoot()
{
	timestamp_t timestamp = ((timestamp_t)time_globalSecs << 32) | time_globalMilliSecs;
	return timestamp;
}

timestamp_t timestamp_getDifference(timestamp_t timestamp1, timestamp_t timestamp2)
{
	uint32_t secs1 = timestamp_getSeconds(timestamp1);
	uint32_t secs2 = timestamp_getSeconds(timestamp2);

	int32_t msecs1 = (int32_t)timestamp_getMilliseconds(timestamp1);
	int32_t msecs2 = (int32_t)timestamp_getMilliseconds(timestamp2);

	uint32_t diffSecs = secs1 - secs2;
	int32_t diffMsecs = msecs1 - msecs2;

	if(diffMsecs < 0)
	{
		diffMsecs += 1000;
		diffSecs --;
	}

	timestamp_t timestamp = ((timestamp_t)diffSecs << 32) | diffMsecs;
	return timestamp;
}

// Time calculation

const uint8_t time_daysOfMonth[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const uint16_t time_daysBeforeMonth[16] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365, 0, 0};

bool time_isLeapYear(uint32_t year)
{
	year = (year + 1) % 400; // correct to nearest multiple of 400 years	
	return (0 == (year & 3) && 100 != year && 200 != year && 300 != year);
}

uint16_t time_daysOfYear(uint32_t year)
{
	return time_isLeapYear(year) ? 366 : 365;
}

time_t time_create(uint32_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
	// Convert into UNIX timestamp
	time_t interval = second;
	interval += (minute * 60);
	interval += (hour * 3600);
	interval += ((day - 1) * 24 * 3600);
	interval += ((time_daysBeforeMonth[month - 1] + time_daysOfMonth[month - 1]) * 24 * 3600);

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
	uint32_t year  = (uint32_t)cmos_readData(CMOS_REGISTER_YEAR) + 2000;
	uint32_t month = (uint32_t)cmos_readData(CMOS_REGISTER_MONTH);
	uint32_t day   = (uint32_t)cmos_readData(CMOS_REGISTER_DMONTH);

	uint32_t hour   = (uint32_t)cmos_readData(CMOS_REGISTER_HOUR);
	uint32_t minute = (uint32_t)cmos_readData(CMOS_REGISTER_MINUTE);
	uint32_t second = (uint32_t)cmos_readData(CMOS_REGISTER_SECOND);

	
	time_t interval = time_create(year, month, day, hour, minute, second);
	time_bootTime = ((timestamp_t)interval << 32);
}

void time_waitForBootTime()
{
	if(!time_gotBootTime)
	{
		dbg("\nWaiting for boot time to become available...\n");

		while(!time_gotBootTime)
			__asm__ volatile ("hlt;");
	}

	// Fix the start time of the current processes
	process_t *process = process_getFirstProcess();
	while(process)
	{
		process->startTime = time_bootTime;
		process = process->next;
	}

	dbg("Boot time is %i\n", (int)(time_bootTime >> 32));
}

uint32_t time_tick(uint32_t esp)
{
	// The first tick is invalid because it gets delayed due to interrupts being disabled during a short time of the boot process
	static bool firstTick = true;
	if(firstTick)
	{
		firstTick = false;
		return esp;
	}

	// Check if we already got the boot time, otherwise get the current time from the CMOS
	if(!time_gotBootTime)
	{
		time_readCMOS(); // Its safe to read from the CMOS now
		time_gotBootTime = true;

		return esp;
	}

	// Update the global time
	time_globalMilliSecs += TIME_MILLISECS_PER_TICK;
	if((time_globalMilliSecs % 1000) == 0)
	{
		time_globalMilliSecs = 0;
		time_globalSecs ++;
	}

	// Call the scheduler
	esp = sd_schedule(esp);
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

	ir_setInterruptHandler(time_tick, 0x20);

	dbg("clock speed pinned at %iHz", TIME_FREQUENCY);
	return true;
}
