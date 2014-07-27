//
//  tls.c
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2014 by Sidney Just
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

#include "tls.h"
#include "spinlock.h"

#ifndef __KERNEL

#define kTLSMaxKeys 64

static unsigned long long _tls_bitmap = 0;
static spinlock_t _tls_lock = SPINLOCK_INIT;

static inline unsigned int *tls_get_buckets()
{
	int address;
	__asm__ volatile("mov %%gs, %%ax" : "=a" (address));

	return (unsigned int *)address;
}

int tls_set(tls_key_t key, const void *value)
{
	if(key == (tls_key_t)-1 || key == 0 || key >= kTLSMaxKeys)
		return 0;

	unsigned int *bucket = tls_get_buckets();
	int result = 0;

	spinlock_lock(&_tls_lock);

	if(_tls_bitmap & (1LL << key))
	{
		bucket[key] = (unsigned int)value;
		result = 1;
	}

	spinlock_unlock(&_tls_lock);

	return result;
}
void *tls_get(tls_key_t key)
{
	if(key == (tls_key_t)-1 || key == 0 || key >= kTLSMaxKeys)
		return (void *)0x0;

	unsigned int *bucket = tls_get_buckets();
	unsigned int result = 0;
	
	spinlock_lock(&_tls_lock);

	if(_tls_bitmap & (1LL << key))
		result = bucket[key];

	spinlock_unlock(&_tls_lock);

	return (void *)result;
}

tls_key_t tls_allocateKey()
{
	spinlock_lock(&_tls_lock);

	for(int i = 1; i < kTLSMaxKeys; i ++)
	{
		if((_tls_bitmap & (1LL << i)) == 0)
		{
			_tls_bitmap |= (1LL << i);
			spinlock_unlock(&_tls_lock);

			return i;
		}
	}

	spinlock_unlock(&_tls_lock);

	return -1;
}
void tls_freeKey(tls_key_t key)
{
	if(key == (tls_key_t)-1 || key == 0 || key >= kTLSMaxKeys)
		return;

	unsigned int *bucket = tls_get_buckets();

	spinlock_lock(&_tls_lock);

	if(_tls_bitmap & (1LL << key))
	{
		_tls_bitmap &= ~(1LL << key);
		bucket[key] = 0;
	}

	spinlock_unlock(&_tls_lock);
}



int *__tls_errno()
{
	unsigned int *bucket = tls_get_buckets();
	return (int *)bucket;
}
void __tls_setErrno(int value)
{
	*__tls_errno() = value;
}

#endif
