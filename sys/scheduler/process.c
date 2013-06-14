//
//  process.c
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

#include <errno.h>
#include <memory/memory.h>
#include <system/syslog.h>
#include <syscall/scmmap.h>
#include <libc/string.h>
#include <interrupts/trampoline.h>
#include <vfs/vfs.h>
#include "process.h"
#include "scheduler.h"

extern void _process_setFirstProcess(process_t *process); // Scheduler.c
extern process_t *process_getFirstProcess();

extern thread_t *thread_clone(struct process_s *target, thread_t *source, int *errno);
bool process_duplicateFiles(process_t *process, process_t *source, int *errno);

void thread_destroy(thread_t *thread);
void process_destroy(process_t *process);

pid_t _process_getUniqueID()
{
	static pid_t uid = 0;
	return uid ++;
}

process_t *process_createVoid(int *errno)
{
	process_t *process = (process_t *)halloc(NULL, sizeof(process_t));
	if(process)
	{
		process_t *parent = process_getCurrentProcess();

		process->pid    = _process_getUniqueID();
		process->died   = false;
		process->ring0  = false;
		process->blocks = 0;

		process->lock  = SPINLOCK_INIT;

		process->references = 1; // Each process is by default references by their parents

		process->parent 	= parent ? parent->pid : PROCESS_NULL;
		process->pprocess 	= parent;

		process->image       = NULL;
		process->pdirectory  = NULL;
		process->context     = NULL;

		process->startTime = time_getTimestamp();
		process->usedTime  = 0;

		process->mappings = atree_create(mmap_atreeLookup);
		if(!process->mappings)
		{
			hfree(NULL, process);
			process = NULL;

			goto process_outro;
		}

		memset(&process->files, 0, kSDMaxOpenFiles * sizeof(vfs_file_t *));
		process->openFiles = 0;

		process->threadCounter = 0;
		process->mainThread    = NULL;
		process->next          = NULL;

		if(!process->mappings)
		{
			atree_destroy(process->mappings);
			hfree(NULL, process);
			process = NULL;

			goto process_outro;
		}
	}

process_outro:

	if(!process && errno)
		*errno = ENOMEM;

	return process;
}

void process_insert(process_t *process)
{
	sd_lock();

	process_t *parent = process->pprocess;

	process->next = parent->next;
	parent->next  = process;

	sd_unlock();
}

#define __process_assert(condition, p, error) if(!(condition)) { \
		warn("__process_assert(%s) failed!\n", #condition); \
		process_destroy(p); \
		if(errno) \
			*errno = error; \
		return NULL; \
	}

process_t *process_create(int *errno)
{
	process_t *process = process_createVoid(errno);
	if(process)
	{
		process->pdirectory = vm_createDirectory();
		__process_assert(process->pdirectory, process, ENOMEM);

		process->image = ld_exectuableCreate(process->pdirectory);
		__process_assert(process->image, process, ENOMEM);

		process->context = vfs_contextCreate(process->pdirectory);
		__process_assert(process->context, process, ENOMEM);

		thread_t *thread = thread_create(process, (thread_entry_t)process->image->entry, 4 * 4096, errno, 0);
		if(!thread)
		{
			process_destroy(process);
			return NULL;
		}

		process->blocks = 1;
		process_insert(process);
	}

	return process;
}

process_t *process_fork(process_t *parent, int *errno)
{
	process_t *child = process_createVoid(errno);
	if(child)
	{
		child->pdirectory = vm_createDirectory();
		__process_assert(child->pdirectory, child, ENOMEM);

		child->image = ld_exectuableCopy(child->pdirectory, parent->image);
		__process_assert(child->image, child, ENOMEM);

		child->context = vfs_contextCreate(child->pdirectory);
		__process_assert(child->context, child, ENOMEM);

		child->context->chdir = parent->context->chdir;

		thread_t *thread = thread_clone(child, parent->scheduledThread, errno);
		if(!thread)
		{
			process_destroy(child);
			return NULL;
		}

		process_duplicateFiles(child, parent, errno);
		mmap_copyMappings(child, parent);

		process_insert(child);
	}

	return child;
}

process_t *process_forgeInitialProcess()
{
	process_t *process = process_createVoid(NULL);
	if(process)
	{		
		process->pdirectory = vm_getKernelDirectory();
		process->ring0      = true;
		process->context    = vfs_getKernelContext();

		thread_create(process, NULL, 4096, NULL, 0);
		_process_setFirstProcess(process);
	}

	return process;
}


process_t *process_getParent()
{
	process_t *process = process_getCurrentProcess();
	return process->pprocess;
}

process_t *process_getWithPid(pid_t pid)
{
	sd_lock();

	process_t *process = process_getFirstProcess();
	while(process)
	{
		if(process->pid == pid)
			break;

		process = process->next;
	}

	sd_unlock();
	return process;
}


void process_closeFiles(process_t *process)
{
	for(int i=0; i<kSDMaxOpenFiles; i++)
	{
		if(process->files[i] && process->files[i] != kSDInvalidFile)
		{
			vfs_forceClose(process->context, process, process->files[i]);
		}

		process->files[i] = NULL;
	}
}

bool process_duplicateFiles(process_t *process, process_t *source, int *errno)
{
	bool result = true;

	for(int i=0; i<kSDMaxOpenFiles; i++)
	{
		if(source->files[i] && source->files[i] != kSDInvalidFile)
		{
			process->files[i] = vfs_duplicateFile(process->context, source, source->files[i], errno);
			if(!process->files[i])
			{
				result = false;
				break;
			}
		}
	}

	return result;
}

void process_switchExecutable(process_t *process, const char *file, int *errno)
{
	process_lock(process);
	process_block(process);

	// Replace the linker...
	//ld_destroyExecutable(process->image);
	//process->image = ld_exectuableCreate(process->pdirectory);

	// General clean up
	//process_closeFiles(process);
	mmap_destroyMappings(process);

	// Get rid of all threads except of the currently scheduled thread
	thread_t *mthread = process->scheduledThread;
	thread_t *thread = process->mainThread;
	while(thread)
	{
		if(thread != mthread)
		{
			thread_t *temp = thread->next;
			thread_destroy(thread);

			thread = temp;
			continue;
		}

		thread = thread->next;
	}

	// Make the scheduled thread the main thread
	process->mainThread = mthread;
	mthread->next = NULL;

	thread_reentry(process->mainThread, (thread_entry_t)process->image->entry, 0);

	// Last but not least, clean the thread
	process_unlock(process);
	process_unblock(process);
}



void process_destroy(process_t *process)
{
	// Dereference all children
	{
		process_t *temp = process_getFirstProcess();
		while(temp)
		{
			if(temp->pprocess == process)
				temp->references --;

			temp = temp->next;
		}
	}

	// Clean up all threads
	{
		thread_t *thread = process->mainThread;
		thread_t *temp;
		while(thread)
		{
			temp = thread;
			thread = thread->next;
			
			thread_destroy(temp);
		}
	}

	if(process->image)
		ld_destroyExecutable(process->image);

	if(process->context)
	{
		process_closeFiles(process);
		vfs_contextDelete(process->context);
	}

	// Remove all mappings
	if(process->mappings)
	{
		mmap_destroyMappings(process);
		atree_destroy(process->mappings);
	}

	vm_deleteDirectory(process->pdirectory);
	hfree(NULL, process);
}


void process_lock(process_t *process)
{
	spinlock_lock(&process->lock);
}
void process_unlock(process_t *process)
{
	spinlock_unlock(&process->lock);
}
bool process_tryLock(process_t *process)
{
	return spinlock_tryLock(&process->lock);
}


void process_block(process_t *process)
{
	process->blocks ++;	
}
void process_unblock(process_t *process)
{
	process->blocks --;
}


int process_allocateFiledescriptor(process_t *process)
{
	for(int i=0; i<kSDMaxOpenFiles; i++)
	{
		if(process->files[i] == NULL)
		{
			process->files[i] = kSDInvalidFile;
			process_unlock(process);

			return i;
		}
	}

	return -1;
}
void process_setFileForFiledescriptor(process_t *process, int fd, struct vfs_file_s *file)
{
	if(fd >= kSDMaxOpenFiles)
		return;

	process->files[fd] = file;
}
void process_releaseFiledescriptor(process_t *process, int fd)
{
	if(fd >= kSDMaxOpenFiles)
		return;

	process->files[fd] = NULL;
}
struct vfs_file_s *process_fileWithFiledescriptor(process_t *process, int fd)
{
	if(fd >= kSDMaxOpenFiles)
		return NULL;

	return process->files[fd];
}
