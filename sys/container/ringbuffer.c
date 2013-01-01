//
//  ringbuffer.c
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

#include <memory/memory.h>
#include <libc/math.h>
#include "ringbuffer.h"

ringbuffer_t *ringbuffer_create(size_t size)
{
	ringbuffer_t *ringbuffer = halloc(NULL, sizeof(ringbuffer_t));
	uint8_t *buffer = (uint8_t *)halloc(NULL, size);

	if(!buffer || !ringbuffer)
	{
		if(ringbuffer)
			hfree(NULL, ringbuffer);

		if(buffer)
			hfree(NULL, buffer);

		return NULL;
	}

	ringbuffer->buffer      = ringbuffer->begin = ringbuffer->end = buffer;
	ringbuffer->bufferLimit = ringbuffer->buffer + size;

	ringbuffer->size  = size;
	ringbuffer->count = 0;

	return ringbuffer;
}

void ringbuffer_destroy(ringbuffer_t *ringbuffer)
{
	hfree(NULL, ringbuffer->buffer);
	hfree(NULL, ringbuffer);
}


static inline uint8_t *ringbuffer_cycle(uint8_t *val, uint8_t *lower, uint8_t *upper)
{
	return val <= lower ? upper : (val >= upper ? lower : val);
}

void ringbuffer_write(ringbuffer_t *ringbuffer, uint8_t *data, size_t size)
{
	for(size_t i=0; i<size; i++)
	{
		*(ringbuffer->end ++) = data[i];

		ringbuffer->end   = ringbuffer_cycle(ringbuffer->end, ringbuffer->buffer, ringbuffer->bufferLimit);
		ringbuffer->count = MIN(ringbuffer->count + 1, ringbuffer->size);

		if(ringbuffer->count == ringbuffer->size)
			ringbuffer->begin = ringbuffer->end; 
	}
}

size_t ringbuffer_read(ringbuffer_t *ringbuffer, uint8_t *buffer, size_t size)
{
	size_t read = 0;
	while(ringbuffer->count > 0 && read < size)
	{
		buffer[read ++] = *(ringbuffer->begin ++);

		ringbuffer->begin = ringbuffer_cycle(ringbuffer->begin, ringbuffer->buffer, ringbuffer->bufferLimit);
		ringbuffer->count --;
	}

	return read;
}

size_t ringbuffer_length(ringbuffer_t *ringbuffer)
{
	return ringbuffer->count;
}
