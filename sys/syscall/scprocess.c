//
//  scprocess.c
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

uint32_t _sc_exit(uint32_t *esp, uint32_t *UNUSED(uesp), int *UNUSED(errno))
{
	process_t *process = process_getCurrentProcess();
	process->died = true;

	*esp = sd_schedule(*esp);

	return 0; // The process won't receive it...
}

uint32_t _sc_processCreate(uint32_t *UNUSED(esp), uint32_t *uesp, int *errno)
{
	vm_address_t virtual;
	char *name = sc_mapProcessMemory(*(void **)(uesp), &virtual, 2, errno);
	if(!name)
		return -1;


	process_t *process = process_createWithFile(name, errno);
	vm_free(vm_getKernelDirectory(), virtual, 2);

	return process ? (uint32_t)process->pid : -1;
}

uint32_t _sc_processKill(uint32_t *esp, uint32_t *uesp, int *errno)
{
	pid_t pid = *(pid_t *)(uesp);
	process_t *process = process_getWithPid(pid);

	if(process)
	{
		process->died = true;

		*esp = sd_schedule(*esp);
		return 1;
	}


	if(errno)
		*errno = EINVAL;

	return 0;
}

uint32_t _sc_fork(uint32_t *esp, uint32_t *UNUSED(uesp), int *errno)
{
	process_t *process = process_getCurrentProcess();
	process->scheduledThread->esp = *esp; // Update the kernel stack for the thread because its needed when the thread is copied

	process_t *child = process_fork(process, errno);
	if(child)
	{
		cpu_state_t *state = (cpu_state_t *)child->scheduledThread->esp;
		state->eax = 0; // Return 0 to the child

		return child->pid;
	}

	return -1;
}



void _sc_processInit()
{
	sc_setSyscallHandler(SYS_EXIT, _sc_exit);
	sc_setSyscallHandler(SYS_PROCESSCREATE, _sc_processCreate);
	sc_setSyscallHandler(SYS_PROCESSKILL, _sc_processKill);
	sc_setSyscallHandler(SYS_FORK, _sc_fork);
}
