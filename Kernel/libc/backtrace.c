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
#include <system/syslog.h>

#define FramePointerLinkOffset 1
#define	FramePointerIsAligned(a1) ((((uintptr_t)a1) & 0x1) == 0)

void kernel_stack_backtrace(vm_address_t *buffer, uint32_t max, uint32_t *nb, uint32_t skip)
{
    void *frame, *next;
    *nb = 0;
    frame = __builtin_frame_address(0);

    if(!FramePointerIsAligned(frame))
		return;


    for(; skip > 0; skip --)
    {
		next = *(void **)frame;

		if(!FramePointerIsAligned(next) || next <= frame)
	    	return;

		frame = next;
    }

    for(; max > 0; max --)
    {
    	vm_address_t offset = *(vm_address_t *)(((void **)frame) + FramePointerLinkOffset);

        buffer[(*nb)++] = offset;
		next = *(void **)frame;

		if(!FramePointerIsAligned(next) || next <= frame)
	    	return;

		frame = next;
    }
}

int backtrace(void **buffer, int size)
{
	uint32_t frames;
	kernel_stack_backtrace((vm_address_t *)buffer, size, &frames, 1);

	while(frames >= 1 && buffer[frames-1] == NULL) 
		frames --;

	return frames;
}
