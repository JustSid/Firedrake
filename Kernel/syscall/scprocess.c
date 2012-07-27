//
//  scprocess.c
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

#include <scheduler/scheduler.h>
#include <system/syslog.h>
#include <system/panic.h>
#include "syscall.h"

uint32_t _sc_exit(uint32_t *esp, uint32_t *uesp, int *errno)
{
	process_t *process = process_getCurrentProcess();
	process->died = true;

	*esp = _sd_schedule(*esp);

	return 0; // The process won't receive it...
}

uint32_t _sc_sleep(uint32_t *esp, uint32_t *uesp, int *errno)
{
	thread_t *thread = thread_getCurrentThread();
	thread->usedTicks = thread->wantedTicks + 1;

	*esp = _sd_schedule(*esp);

	return 0;
}

uint32_t _sc_threadAttach(uint32_t *esp, uint32_t *uesp, int *errno)
{
	process_t *process = process_getCurrentProcess();
	uint32_t entry = *(uint32_t *)(uesp + 0);
	uint32_t stacksize = *(uint32_t *)(uesp + 1);

	uint32_t arg1 = *(uint32_t *)(uesp + 2);
	uint32_t arg2 = *(uint32_t *)(uesp + 3);

	thread_t *thread = thread_create(process, (thread_entry_t)entry, stacksize, 2, arg1, arg2);
	return thread->id;
}

uint32_t _sc_threadJoin(uint32_t *esp, uint32_t *uesp, int *errno)
{
	uint32_t tid = *(uint32_t *)(uesp);
	thread_join(tid);

	*esp = _sd_schedule(*esp);

	return 0;
}

uint32_t _sc_threadExit(uint32_t *esp, uint32_t *uesp, int *errno)
{
	thread_t *thread = thread_getCurrentThread();
	thread->died = true;

	*esp = _sd_schedule(*esp);
	return 0;
}

uint32_t _sc_processCreate(uint32_t *esp, uint32_t *uesp, int *errno)
{
	process_t *current = process_getCurrentProcess();
	const char *name = *(const char **)(uesp);

	vm_address_t virtual = round4kDown((vm_address_t)name);
	vm_address_t offset = ((vm_address_t)name) - virtual;
	uintptr_t physical = vm_resolveVirtualAddress(current->pdirectory, virtual);

	virtual = vm_alloc(vm_getKernelDirectory(), physical, 2, VM_FLAGS_KERNEL); // TODO: Look if it really spans over two pages, not just assume anything
	name = (const char *)(virtual + offset);

	process_t *process = process_createWithFile(name);

	vm_free(vm_getKernelDirectory(), virtual, 2); // Unmap the name
	return process ? process->pid : PROCESS_NULL;
}

uint32_t _sc_processKill(uint32_t *esp, uint32_t *uesp, int *errno)
{
	uint32_t pid = *(uint32_t *)(uesp);
	process_t *process = process_getFirstProcess();

	while(process)
	{
		if(process->pid == pid)
		{
			process->died = true;

			*esp = _sd_schedule(*esp);
			return 1;
		}

		process = process->next;
	}

	return 0;
}



uint32_t _sc_fork(uint32_t *esp, uint32_t *uesp, int *errno)
{
	process_t *process = process_getCurrentProcess();
	process->scheduledThread->esp = *esp; // Update the kernel stack for the thread because its needed when the thread is copied

	process_t *child = process_fork(process);

	if(child)
	{
		cpu_state_t *state = (cpu_state_t *)child->scheduledThread->esp;
		state->eax = 0; // Return 0 to the child

		return child->pid;
	}

	*errno = EAGAIN; // This is just a guess as its currently impossible to get an actual error from process_fork()
	return -1;
}



void _sc_processInit()
{
	sc_setSyscallHandler(SYS_EXIT, _sc_exit);
	sc_setSyscallHandler(SYS_SLEEP, _sc_sleep);
	sc_setSyscallHandler(SYS_THREADATTACH, _sc_threadAttach);
	sc_setSyscallHandler(SYS_THREADEXIT, _sc_threadExit);
	sc_setSyscallHandler(SYS_THREADJOIN, _sc_threadJoin);
	sc_setSyscallHandler(SYS_PROCESSCREATE, _sc_processCreate);
	sc_setSyscallHandler(SYS_PROCESSKILL, _sc_processKill);
	sc_setSyscallHandler(SYS_FORK, _sc_fork);
}
