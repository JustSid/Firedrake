//
//  syslogd.c
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

#include <memory/memory.h>
#include <container/ringbuffer.h>
#include <scheduler/scheduler.h>
#include <system/lock.h>
#include <system/panic.h>
#include <system/video.h>
#include <libc/string.h>

#include "syslogd.h"
#include <system/syslog.h>

static spinlock_t syslogd_lock = SPINLOCK_INIT;
static ringbuffer_t *syslogd_buffer = NULL;
static bool syslogd_running = false;

static syslog_level_t __syslog_level = LOG_WARNING;
static vd_color_t __sylog_color_table[] = {
	vd_color_red,			// LOG_ALERT
	vd_color_lightRed, 		// LOG_CRITICAL
	vd_color_brown, 		// LOG_ERROR
	vd_color_yellow, 		// LOG_WARNING
	vd_color_lightGray, 	// LOG_INFO
	vd_color_lightBlue 		// LOG_DEBUG
};


void syslogd_queueMessage(syslog_level_t level, const char *message)
{
	if(level > __syslog_level)
		return;

	spinlock_lock(&syslogd_lock);

	if(!syslogd_running)
	{
		vd_setColor(__sylog_color_table[level], true);
		vd_writeString(message);

		spinlock_unlock(&syslogd_lock);
		return;
	}

	size_t length = strlen(message);
	uint8_t colorbuffer[2] = { 14, vd_translateColor(__sylog_color_table[level]) };

	ringbuffer_write(syslogd_buffer, colorbuffer, 2);
	ringbuffer_write(syslogd_buffer, (uint8_t *)message, length);

	spinlock_unlock(&syslogd_lock);
}

void syslogd_setLogLevel(syslog_level_t level)
{
	__syslog_level = level;
}



void __syslogd_flush()
{
	size_t length = ringbuffer_length(syslogd_buffer);
	while(length > 0)
	{
		char buffer[256];
		size_t read = ringbuffer_read(syslogd_buffer, (uint8_t *)buffer, 255);

		buffer[read] = '\0';
		length = ringbuffer_length(syslogd_buffer);

		vd_writeString(buffer);
	}
}

void syslogd_flush()
{
	if(!syslogd_running)
		return;

	spinlock_lock(&syslogd_lock);

	__syslogd_flush();

	spinlock_unlock(&syslogd_lock);
}

// Evil as fuck in case the lock is currently held by someone...
void syslogd_forceFlush()
{
	if(!syslogd_running)
		return;

	spinlock_unlock(&syslogd_lock);
	syslogd_flush();

	syslogd_running = false;
}

void syslogd()
{
	thread_setName(thread_getCurrentThread(), "syslogd");

	syslogd_buffer = ringbuffer_create(80 * 25);
	if(!syslogd_buffer)
		panic("Couldn't allocate buffer for syslogd!");

	spinlock_lock(&syslogd_lock);
	syslogd_running = true;
	spinlock_unlock(&syslogd_lock);

	while(1)
	{
		syslogd_flush();
		sd_yield();
	}
}
