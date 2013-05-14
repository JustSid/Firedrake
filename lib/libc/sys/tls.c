//
//  sys/tls.c
//  libc
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

#include "syscall.h"
#include "tls.h"

#define kTLSAreaSize 2
#define kTLSBucketCount ((kTLSAreaSize * 4096) / sizeof(struct tls_bucket_s))

struct tls_bucket_s
{
	bool occupied;
	void *value;
};

bool tls_set(tls_key_t key, const void *value)
{
	if(key == kTLSInvalidKey || key == 0)
		return false;

	struct tls_bucket_s *bucket = (struct tls_bucket_s *)syscall(SYS_TLS_AREA, kTLSAreaSize);
	if(bucket)
	{
		bucket += key;
		if(bucket->occupied)
		{
			bucket->value = (void *)value;
			return true;
		}
	}

	return false;
}

void *tls_get(tls_key_t key)
{
	if(key == kTLSInvalidKey || key == 0)
		return NULL;

	struct tls_bucket_s *bucket = (struct tls_bucket_s *)syscall(SYS_TLS_AREA, kTLSAreaSize);
	if(bucket)
	{
		bucket += key;

		if(bucket->occupied)
			return bucket->value;
	}

	return NULL;
}


int *__tls_errno()
{
	struct tls_bucket_s *buckets = (struct tls_bucket_s *)syscall(SYS_TLS_AREA, kTLSAreaSize);
	return (int *)&(buckets[0].value);
}

void __tls_setErrno(int value)
{
	*__tls_errno() = value;
}


tls_key_t tls_allocateKey()
{
	struct tls_bucket_s *buckets = (struct tls_bucket_s *)syscall(SYS_TLS_AREA, kTLSAreaSize);
	if(buckets)
	{
		for(uint32_t i=1; i<kTLSBucketCount; i++)
		{
			if(!buckets[i].occupied)
			{
				buckets[i].occupied = true;
				return (tls_key_t)i;
			}
		}
	}

	return -1;
}

void tls_freeKey(tls_key_t key)
{
	if(key == kTLSInvalidKey || key == 0)
		return;

	struct tls_bucket_s *bucket = (struct tls_bucket_s *)syscall(SYS_TLS_AREA, kTLSAreaSize);
	if(bucket)
	{
		bucket += key;
		bucket->occupied = false;
	}
}
