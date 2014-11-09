//
//  backtrace.c
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

#include "sys/types.h"
#include "backtrace.h"

#define	IsFrameAligned(x) ((((uintptr_t)x) & 0x1) == 0)

struct StackFrame
{
	struct StackFrame *next;
	void *ret;
};

void _stacktrace(void *ebp, void **buffer, int max, int *numOut, int skip)
{
	struct StackFrame *frame = (struct StackFrame *)ebp;
	*numOut = 0;

	if(!IsFrameAligned(frame))
		return;

	for(; skip > 0; skip --)
	{
		if(!IsFrameAligned(frame->next) || frame->next <= frame)
			return;

		frame = frame->next;
	}

	for(; max > 0; max --)
	{
		buffer[(*numOut) ++] = frame->ret;

		if(!IsFrameAligned(frame->next) || frame->next <= frame)
			return;

		frame = frame->next;
	}
}

int backtrace_np(void *ebp, void **buffer, int size)
{
	if(!ebp)
		return 0;

	int frames;
	_stacktrace(ebp, buffer, size, &frames, 1);

	while(frames >= 1 && !buffer[frames-1]) 
		frames --;

	return frames;
}

int backtrace(void **buffer, int size)
{
	void *ebp = __builtin_frame_address(0);
	return backtrace_np(ebp, buffer, size);
}
