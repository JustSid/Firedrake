//
//  scthread.c
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

#include <scheduler/scheduler.h>
#include <system/syslog.h>
#include <system/panic.h>
#include "syscall.h"

uint32_t _sc_yield(uint32_t *esp, uint32_t *UNUSED(uesp), int *UNUSED(errno))
{
	thread_t *thread = thread_getCurrentThread();
	thread->blocks ++;

	*esp = sd_schedule(*esp);

	thread->blocks --;
	return 0;
}

uint32_t _sc_sleep(uint32_t *esp, uint32_t *uesp, int *UNUSED(errno))
{
	thread_t *thread = thread_getCurrentThread();
	uint32_t time = *(uint32_t *)(uesp + 0);

	thread_sleep(thread, time);

	*esp = sd_schedule(*esp);

	return 0;
}

uint32_t _sc_threadAttach(uint32_t *UNUSED(esp), uint32_t *uesp, int *errno)
{
	process_t *process = process_getCurrentProcess();
	uint32_t entry = *(uint32_t *)(uesp + 0);
	uint32_t stacksize = *(uint32_t *)(uesp + 1);

	uint32_t arg1 = *(uint32_t *)(uesp + 2);
	uint32_t arg2 = *(uint32_t *)(uesp + 3);

	thread_t *thread = thread_create(process, (thread_entry_t)entry, stacksize, errno, 2, arg1, arg2);
	return thread ? thread->id : -1;
}

uint32_t _sc_threadJoin(uint32_t *esp, uint32_t *uesp, int *errno)
{
	uint32_t tid = *(uint32_t *)(uesp);
	thread_t *thread = thread_getWithID(tid);

	if(!thread)
	{
		*errno = EINVAL;
		return -1;
	}


	thread_join(thread_getCurrentThread(), thread, errno);

	*esp = sd_schedule(*esp);
	return 0;
}

uint32_t _sc_threadExit(uint32_t *esp, uint32_t *UNUSED(uesp), int *UNUSED(errno))
{
	thread_t *thread = thread_getCurrentThread();
	thread->died = true;

	*esp = sd_schedule(*esp);
	return 0;
}

uint32_t _sc_threadErrno(uint32_t *UNUSED(esp), uint32_t *UNUSED(uesp), int *UNUSED(errno))
{
	thread_t *thread = thread_getCurrentThread();
	uint32_t errno = thread->errno;

	thread->errno = 0;
	return errno;
}

uint32_t _sc_threadTSLAllocate(uint32_t *UNUSED(esp), uint32_t *UNUSED(uesp), int *errno)
{
	thread_t *thread = thread_getCurrentThread();
	return thread_allocateTLSSlot(thread, errno);
}

uint32_t _sc_threadTSLFree(uint32_t *UNUSED(esp), uint32_t *uesp, int *errno)
{
	thread_t *thread = thread_getCurrentThread();
	uint32_t slot = *(uint32_t *)(uesp + 0);

	thread_freeTLSSlot(thread, slot, errno);
	return 0;
}

uint32_t _sc_threadTSLGet(uint32_t *UNUSED(esp), uint32_t *uesp, int *errno)
{
	thread_t *thread = thread_getCurrentThread();
	uint32_t slot = *(uint32_t *)(uesp + 0);

	return thread_getTLSValue(thread, slot, errno);
}

uint32_t _sc_threadTSLSet(uint32_t *UNUSED(esp), uint32_t *uesp, int *errno)
{
	thread_t *thread = thread_getCurrentThread();
	uint32_t slot  = *(uint32_t *)(uesp + 0);
	uint32_t value = *(uint32_t *)(uesp + 1);

	thread_setTLSValue(thread, slot, value, errno);
	return 0;
}

uint32_t _Sc_threadSelf(uint32_t *UNUSED(esp), uint32_t *UNUSED(uesp), int *UNUSED(errno))
{
	thread_t *thread = thread_getCurrentThread();
	return thread->id;
}


void _sc_threadInit()
{
	sc_setSyscallHandler(SYS_THREADYIELD, _sc_yield);
	sc_setSyscallHandler(SYS_THREADSLEEP, _sc_sleep);
	sc_setSyscallHandler(SYS_THREADATTACH, _sc_threadAttach);
	sc_setSyscallHandler(SYS_THREADEXIT, _sc_threadExit);
	sc_setSyscallHandler(SYS_THREADJOIN, _sc_threadJoin);
	sc_setSyscallHandler(SYS_THREADSELF, _Sc_threadSelf);
	sc_setSyscallHandler(SYS_ERRNO, _sc_threadErrno);
	sc_setSyscallHandler(SYS_TLS_ALLOCATE, _sc_threadTSLAllocate);
	sc_setSyscallHandler(SYS_TLS_FREE, _sc_threadTSLFree);
	sc_setSyscallHandler(SYS_TLS_SET, _sc_threadTSLSet);
	sc_setSyscallHandler(SYS_TLS_GET, _sc_threadTSLGet);
}
