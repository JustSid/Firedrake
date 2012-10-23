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

#include <system/video.h>
#include <container/list.h>
#include <memory/memory.h>
#include <scheduler/scheduler.h>
#include <libc/string.h>
#include "syslogd.h"

typedef struct syslogd_message_s
{
	char *message;
	syslog_level_t level;

	struct syslogd_message_s *next;
	struct syslogd_message_s *prev;
} syslogd_message_t;

static bool syslogd_running = false;
static list_t *syslogd_queue = NULL;

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
	if(!syslogd_running)
	{
		if(level > __syslog_level)
			return;

		vd_printString(message, __sylog_color_table[(int)level]);
		return;
	}


	size_t length = strlen(message);
	char *copy    = halloc(NULL, length + 1);
	if(!copy)
		return;

	strcpy(copy, message);


	list_lock(syslogd_queue);
	{
		syslogd_message_t *entry = list_addBack(syslogd_queue);
		entry->message = copy;
		entry->level = level;
	}
	list_unlock(syslogd_queue);
}

void syslogd_setLogLevel(syslog_level_t level)
{
	__syslog_level = level;	
}


void syslogd_flush()
{
	if(!syslogd_running)
		return;

	list_lock(syslogd_queue);

	syslogd_message_t *entry = list_first(syslogd_queue);
	while(entry)
	{
		if(entry->level <= __syslog_level)
			vd_printString(entry->message, __sylog_color_table[(int)entry->level]);
		
		syslogd_message_t *next = entry->next;

		hfree(NULL, entry->message);
		list_remove(syslogd_queue, entry);

		entry = next;
	}

	list_unlock(syslogd_queue);
}

// This function is evil!!
// Only use it from panic as it will force flush the current message queue and create a new one that is definitely unlocked!
// Afterwards none of the previously running syslogd function is re-entrant anymore, make sure no calling process gets CPU time!
void syslogd_forceFlush()
{
	if(!syslogd_running)
		return;

	bool result = spinlock_tryLock(&syslogd_queue->lock);
	if(result)
	{
		spinlock_unlock(&syslogd_queue->lock);
		syslogd_flush();

		return;
	}

	syslogd_message_t *entry = list_first(syslogd_queue);
	while(entry)
	{
		if(entry->message && entry->level <= __syslog_level)
			vd_printString(entry->message, __sylog_color_table[(int)entry->level]);
		
		entry = entry->next;
	}

	// Create a new queue
	syslogd_queue = list_create(sizeof(syslogd_message_t), offsetof(syslogd_message_t, next), offsetof(syslogd_message_t, prev));
}

void syslogd()
{
	syslogd_queue = list_create(sizeof(syslogd_message_t), offsetof(syslogd_message_t, next), offsetof(syslogd_message_t, prev));
	syslogd_running = true;

	while(1)
	{
		syslogd_flush();
		sd_yield();
	}
}
