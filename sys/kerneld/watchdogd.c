//
//  watchdog.c
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

#include <container/array.h>
#include <container/hashset.h>
#include <libc/string.h>
#include <system/time.h>
#include <system/lock.h>
#include <system/kernel.h>
#include <system/syslog.h>

#include "watchdogd.h"

#define kWatchdogdSampleRate   5
#define kWatchdogdMaxSamples   50
#define kWatchdogdPrintSamples 15

typedef struct
{
	uint32_t count;
	const char *name;
	timestamp_t lastSeen;
} watchdogd_sample_t;

typedef struct
{
	thread_t *thread;
	hashset_t *samples;

	uint32_t eip;
	uint32_t eipHitCount;

	uint32_t totalSamples;
	uint32_t keptSamples;

	timestamp_t nextCheck;
} watchdogd_thread_t;

static array_t *watchdogd_watchedThreads = NULL;
static spinlock_t watchdogd_spinlock;



comparison_result_t __watchdogd_sampleSort(void *t1, void *t2)
{
	watchdogd_sample_t *sample1 = (watchdogd_sample_t *)t1;
	watchdogd_sample_t *sample2 = (watchdogd_sample_t *)t2;

	if(sample1->count < sample2->count)
		return kCompareGreaterThan;

	if(sample1->count > sample2->count)
		return kCompareLesserThan;

	return kCompareEqualTo;
}

void __watchdogd_printThreadResults(watchdogd_thread_t *wthread)
{
	if(wthread->totalSamples == 0)
		return;
	
	array_t *samples = hashset_allData(wthread->samples);
	array_sort(samples, __watchdogd_sampleSort);

	dbg("Watchdog sampling results for %i:%i. ", wthread->thread->process->pid, wthread->thread->id);
	dbg("Total samples %i, kept %i\n", wthread->totalSamples, wthread->keptSamples);

	dbg("Rank:  Function:                               Count:\n");

	for(uint32_t i=0; i<array_count(samples); i++)
	{
		if(i >= kWatchdogdPrintSamples)
			break;

		watchdogd_sample_t *sample = array_objectAtIndex(samples, i);

		size_t length = strlen(sample->name);
		size_t padding = 40 - length;

		info(" %02i     %s", (i + 1), sample->name);

		while(padding > 0)
		{
			info(" ");
			padding --;
		}

		info("%-5i\n", sample->count);
	}

	info("\n");
	array_destroy(samples);
}

void watchdogd_printSamplingResults(thread_t *thread)
{
	spinlock_lock(&watchdogd_spinlock);

	for(uint32_t i=0; i<array_count(watchdogd_watchedThreads); i++)
	{
		watchdogd_thread_t *wthread = array_objectAtIndex(watchdogd_watchedThreads, i);
		if(wthread->thread == thread)
		{
			spinlock_unlock(&watchdogd_spinlock);
			__watchdogd_printThreadResults(wthread);

			return;
		}
	}

	spinlock_unlock(&watchdogd_spinlock);
}


void watchdogd_sampleThread(thread_t *thread, watchdogd_thread_t *wthread)
{
	uint32_t eip;
	thread->blocked ++;
	{
		cpu_state_t *state = (cpu_state_t *)thread->esp;
		eip = state->eip;
	}
	thread->blocked --;


	uintptr_t address = kern_resolveAddress(eip);
	const char *name  = kern_nameForAddress(address, NULL);

	watchdogd_sample_t *sample = hashset_dataForKey(wthread->samples, (void *)name);
	if(!sample)
	{
		sample = halloc(NULL, sizeof(watchdogd_sample_t *));
		sample->count = 0;
		sample->name  = name;

		hashset_setDataForKey(wthread->samples, sample, (void *)name);

		if(hashset_count(wthread->samples) > kWatchdogdMaxSamples)
		{
			watchdogd_sample_t *leastFavorite = NULL;

			for(uint32_t i=0; i<wthread->samples->capacity; i++)
			{
				hashset_bucket_t *bucket = wthread->samples->buckets[i];
				while(bucket)
				{
					if(bucket->data && bucket->data != sample)
					{
						watchdogd_sample_t *tsample = (watchdogd_sample_t *)bucket->data;
						if(!leastFavorite)
						{
							leastFavorite = tsample;
							bucket = bucket->overflow;

							continue;
						}

						if(leastFavorite->lastSeen > tsample->lastSeen)
							leastFavorite = tsample;
					}

					bucket = bucket->overflow;
				}
			}

			wthread->keptSamples -= leastFavorite->count;

			hashset_removeDataForKey(wthread->samples, (void *)leastFavorite->name);
			hfree(NULL, leastFavorite);
		}
	}

	sample->count ++;
	sample->lastSeen = time_getTimestamp();

	wthread->totalSamples ++;
	wthread->keptSamples ++;

	// Deadlock check
	if(wthread->eip != eip || thread->wasNice)
	{
		wthread->eipHitCount = 0;
		wthread->eip = eip;
	}

	wthread->eipHitCount ++;
	if(wthread->eipHitCount == UINT8_MAX)
	{
		process_t *process = thread->process;

		warn("Watchdogd: Thread %i:%i seems to be stuck at %p!\n", process->pid, thread->id, eip);
		kern_printBacktraceForThread(thread, 15);
		info("\n");
	}
}



void watchdogd_addThread(thread_t *thread)
{
	watchdogd_thread_t *wthread = halloc(NULL, sizeof(watchdogd_thread_t));
	if(wthread)
	{
		wthread->samples = hashset_create(0, hash_cstring);
		wthread->thread = thread;

		wthread->nextCheck = 0;

		wthread->eip = 0;
		wthread->eipHitCount = 0;


		spinlock_lock(&watchdogd_spinlock);
		array_addObject(watchdogd_watchedThreads, wthread);
		spinlock_unlock(&watchdogd_spinlock);
	}
}

void watchdogd_removeThread(thread_t *thread)
{
	spinlock_lock(&watchdogd_spinlock);

	for(uint32_t i=0; i<array_count(watchdogd_watchedThreads); i++)
	{
		watchdogd_thread_t *wthread = array_objectAtIndex(watchdogd_watchedThreads, i);
		if(wthread->thread == thread)
		{
			array_removeObjectAtIndex(watchdogd_watchedThreads, i);
			spinlock_unlock(&watchdogd_spinlock);

			__watchdogd_printThreadResults(wthread);

			hashset_destroy(wthread->samples);
			hfree(NULL, wthread);

			return;
		}
	}

	spinlock_unlock(&watchdogd_spinlock);
}

void watchdogd()
{
	watchdogd_removeThread(thread_getCurrentThread()); // We don't watch over ourself

	while(1)
	{
		spinlock_lock(&watchdogd_spinlock);
		timestamp_t current = time_getTimestamp();

		for(uint32_t i=0; i<array_count(watchdogd_watchedThreads); i++)
		{
			watchdogd_thread_t *wthread = array_objectAtIndex(watchdogd_watchedThreads, i);

			if(current >= wthread->nextCheck)
			{
				if(wthread->thread->blocked > 0 || wthread->thread->died)
					continue;
				
				watchdogd_sampleThread(wthread->thread, wthread);
				wthread->nextCheck = current + kWatchdogdSampleRate;
			}
		}

		spinlock_unlock(&watchdogd_spinlock);
		sd_yield();
	}
}

bool watchdogd_init(void *UNUSED(data))
{
	watchdogd_watchedThreads = array_create();
	return (watchdogd_watchedThreads != NULL);
}
