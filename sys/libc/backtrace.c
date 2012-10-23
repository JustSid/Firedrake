//
//  backtrace.c
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

#include <types.h>
#include <memory/memory.h>
#include <scheduler/scheduler.h>
#include <system/syslog.h>

#define FramePointerLinkOffset 1
#define	FramePointerIsAligned(a1) ((((uintptr_t)a1) & 0x1) == 0)

struct stack_frame_s
{
	struct stack_frame_s *next;
	void *ret;
};

void kernel_stacktraceForEBP(void *ebp, void **buffer, uint32_t max, uint32_t *numOut, uint32_t skip)
{
	struct stack_frame_s *frame = (struct stack_frame_s *)ebp;
	*numOut = 0;

	if(!FramePointerIsAligned(frame))
		return;

	for(; skip>0; skip--)
	{
		if(!FramePointerIsAligned(frame->next) || frame->next <= frame)
			return;

		frame = frame->next;
	}

	for(; max>0; max--)
	{
		buffer[(*numOut) ++] = frame->ret;

		if(!FramePointerIsAligned(frame->next) || frame->next <= frame)
			return;

		frame = frame->next;
	}
}

void kernel_stacktrace(void **buffer, uint32_t max, uint32_t *numOut, uint32_t skip)
{
	void *ebp = __builtin_frame_address(0);
	kernel_stacktraceForEBP(ebp, buffer, max, numOut, skip);
}



int backtraceForEBP(void *ebp, void **buffer, int size)
{
	if(!ebp)
		return 0;

	uint32_t frames;
	kernel_stacktraceForEBP(ebp, buffer, (uint32_t)size, &frames, 1);

	while(frames >= 1 && buffer[frames-1] == NULL) 
		frames --;

	return (int)frames;
}

int backtrace(void **buffer, int size)
{
	uint32_t frames;
	kernel_stacktrace(buffer, (uint32_t)size, &frames, 1);

	while(frames >= 1 && buffer[frames-1] == NULL) 
		frames --;

	return (int)frames;
}
