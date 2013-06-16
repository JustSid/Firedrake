//
//  iostubs.c
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

#include <bootstrap/multiboot.h>
#include <container/hashset.h>
#include <container/array.h>
#include <interrupts/interrupts.h>
#include <scheduler/scheduler.h>
#include <memory/memory.h>
#include <system/helper.h>
#include <system/kernel.h>
#include <system/syslog.h>
#include <system/cpu.h>
#include <system/panic.h>
#include <libc/string.h>

#include "iostubs.h"
#include "iostore.h"
#include "ioerror.h"

spinlock_t __io_lazy_lock = SPINLOCK_INIT; // Shared spinlock for lazy allocation
typedef void (*io_libio_callback_t)(void *owner, ...);

void __io_primitiveLog(char *message, bool appendNewline)
{
	const char *format = appendNewline ? "%s\n" : "%s";
	syslog(LOG_INFO, format, message);
}

// ------
// Thread management
// ------

void __io_threadEntry(__unused void *trash, io_libio_callback_t entry, void *owner, void *arg)
{
	entry(owner, arg);
	sd_threadExit();
}

uint32_t __io_threadCreate(void *entry, void *owner, void *arg)
{
	process_t *process = process_getCurrentProcess();
	thread_t *thread   = thread_create(process, __io_threadEntry, 0, NULL, 3, entry, owner, arg);

	return thread->id;
}

uint32_t __io_threadID()
{
	thread_t *thread = thread_getCurrentThread();
	return thread->id;
}

void __io_threadSetName(uint32_t id, const char *name)
{
	thread_t *thread = thread_getWithID(id);
	if(thread)
		thread_setName(thread, name, NULL);
}

void __io_threadSleep(uint32_t id, uint32_t time)
{
	thread_t *thread = thread_getWithID(id);
	if(thread)
	{
		thread_sleep(thread, time);
		sd_yield();
	}
}

void __io_threadWakeup(uint32_t id)
{
	thread_t *thread = thread_getWithID(id);
	if(thread)
		thread_wakeup(thread);
}

// ------
// Memory mamangement
// ------

heap_t *__io_heap = NULL;

void *kern_alloc(size_t size)
{
	spinlock_lock(&__io_lazy_lock);

	if(__io_heap == NULL)
	{
		__io_heap = heap_create(kHeapFlagSecure | kHeapFlagAligned);
		if(!__io_heap)
			panic("Couldn't create heap for libio!");
	}

	spinlock_unlock(&__io_lazy_lock);
	
	void *mem = halloc(__io_heap, size);
	return mem;
}

void kern_free(void *mem)
{
	hfree(__io_heap, mem);
}

// ------
// Interrupts
// ------

typedef struct
{
	io_libio_callback_t callback;
	void *owner;
	void *context;
} io_interrupt_handler_t;

typedef struct
{
	bool exclusive;
	array_t *handler;
} io_interrupt_entry_t;

static spinlock_t __io_interrupt_lock = SPINLOCK_INIT;
static io_interrupt_entry_t *__io_interruptEntries[IDT_ENTRIES];

void __io_dispatchInterrupt(cpu_state_t *state)
{
	io_interrupt_entry_t *entry = __io_interruptEntries[state->interrupt];

	if(entry)
	{
		array_t *allHandler = entry->handler;
		for(size_t i=0; i<array_count(allHandler); i++)
		{
			io_interrupt_handler_t *handler = array_objectAtIndex(allHandler, i);
			handler->callback(handler->owner, handler->context, state->interrupt);
		}
	}
}

IOReturn kern_requestExclusiveInterrupt(void *owner, void *context, uint32_t *outInterrupt, io_libio_callback_t callback)
{
	spinlock_lock(&__io_interrupt_lock);

	uint32_t limit = ir_interruptPublicEnd();
	for(uint32_t i=ir_interruptPublicBegin(); i<limit; i++)
	{
		if(ir_isValidInterrupt(i, true))
		{
			io_interrupt_entry_t *entry = __io_interruptEntries[i];
			if(entry)
				continue;

			entry = halloc(NULL, sizeof(io_interrupt_entry_t));
			entry->exclusive = true;
			entry->handler = array_create();

			__io_interruptEntries[i] = entry;

			io_interrupt_handler_t *handler = halloc(NULL, sizeof(io_interrupt_handler_t));
			handler->owner = owner;
			handler->context = context;
			handler->callback = callback;

			array_addObject(entry->handler, handler);
			spinlock_unlock(&__io_interrupt_lock);

			if(outInterrupt)
				*outInterrupt = i;

			return kIOReturnSuccess;
		}
	}

	spinlock_unlock(&__io_interrupt_lock);
	return kIOReturnNoInterrupt;
}

IOReturn kern_registerForInterrupt(uint32_t interrupt, bool exclusive, void *owner, void *context, io_libio_callback_t callback)
{
	spinlock_lock(&__io_interrupt_lock);

	io_interrupt_handler_t *handler;
	io_interrupt_entry_t *entry = __io_interruptEntries[interrupt];
	if(entry)
	{
		if(!callback)
		{
			for(size_t i=0; i<array_count(entry->handler); i++)
			{
				handler = array_objectAtIndex(entry->handler, i);
				if(handler->owner == owner)
				{
					array_removeObjectAtIndex(entry->handler, i);
					spinlock_unlock(&__io_interrupt_lock);

					return kIOReturnSuccess;
				}
			}

			spinlock_unlock(&__io_interrupt_lock);
			return kIOReturnNoInterrupt;
		}

		if(entry->exclusive || exclusive)
		{
			spinlock_unlock(&__io_interrupt_lock);
			return kIOReturnNoExclusiveAccess;
		}

		handler = halloc(NULL, sizeof(io_interrupt_handler_t));
		if(!handler)
		{
			spinlock_unlock(&__io_interrupt_lock);
			return kIOReturnNoMemory;
		}
	}
	else if(callback)
	{
		handler = halloc(NULL, sizeof(io_interrupt_handler_t));
		entry = halloc(NULL, sizeof(io_interrupt_entry_t));

		if(!handler || !entry)
		{
			if(handler)
				hfree(NULL, handler);

			if(entry)
				hfree(NULL, entry);

			spinlock_unlock(&__io_interrupt_lock);
			return kIOReturnNoMemory;
		}

		entry->exclusive = exclusive;
		entry->handler = array_create();

		__io_interruptEntries[interrupt] = entry;
		ir_setInterruptCallback(__io_dispatchInterrupt, interrupt);
	}

	if(callback)
	{
		handler->owner = owner;
		handler->context = context;
		handler->callback = callback;

		array_addObject(entry->handler, handler);
		spinlock_unlock(&__io_interrupt_lock);

		return kIOReturnSuccess;
	}

	spinlock_unlock(&__io_interrupt_lock);
	return kIOReturnError;
}

// Symbol lookup

// List of exported symbols from the kernel
// The kernel will only allow libraries to link the symbols in this list
const char *__io_exportedSymbolNames[] = {
	"panic",
	"__io_primitiveLog",
	"sys_checkCommandline",
	"__divdi3",
	"__moddi3",
	"__udivdi3",
	"__umoddi3",
	// Threads
	"sd_yield",
	"__io_threadCreate",
	"__io_threadID",
	"__io_threadSetName",
	"__io_threadSleep",
	"__io_threadWakeup",
	// Interrupts
	"kern_requestExclusiveInterrupt",
	"kern_registerForInterrupt",
	// Kernel module handling
	"io_moduleWithName",
	"io_moduleRetain",
	"io_moduleRelease",
	// Memory
	"kern_alloc",
	"kern_free",
	"dma_request",
	"dma_free",
	// Time
	"time_getSeconds",
	"time_getMilliseconds",
	"time_convertUnix",
	"time_convertTimestamp",
	"time_getTimestamp",
	"time_getUnixTime",
	"time_getBootTime"
};

io_library_t *__io_kernelLibrary = NULL;
elf_sym_t **__io_exportedSymbols = NULL;

size_t __io_kernelLibrarySymbolCount;



io_library_t *io_kernelLibraryStub()
{
	return __io_kernelLibrary;
}

elf_sym_t *io_findKernelSymbol(const char *name)
{
	// Look if the symbol was already exported...
	bool isExported = false;
	size_t index = 0;

	size_t size = sizeof(__io_exportedSymbolNames) / sizeof(char *);
	for(size_t i=0; i<size; i++)
	{
		if(strcmp(__io_exportedSymbolNames[i], name) == 0)
		{
			if(__io_exportedSymbols[i] != NULL)
				return __io_exportedSymbols[i];

			index = i;
			isExported = true;
			break;
		}
	}

	// If the symbol isn't exported by the kernel in general, we won't look into the kernels symbol at all
	if(!isExported)
		return NULL;

	// Find the symbol in the binary
	elf_sym_t *symbol = __io_kernelLibrary->symtab;
	for(size_t i=0; i<__io_kernelLibrarySymbolCount; i++, symbol++)
	{
		if(symbol->st_name >= __io_kernelLibrary->strtabSize || symbol->st_name == 0)
			continue;

		const char *symname = __io_kernelLibrary->strtab + symbol->st_name;
		if(strcmp(symname, name) == 0)
		{
			__io_exportedSymbols[index] = symbol;
			return symbol;
		}
	}

	return NULL;
}


bool io_initStubs()
{
	__io_kernelLibrary = halloc(NULL, sizeof(io_library_t));
	if(__io_kernelLibrary)
	{
		// Look for the kernels symbol and string table
		elf_header_t *header = kern_fetchHeader();
		elf_section_header_t *section = (elf_section_header_t *)(((char *)header) + header->e_shoff);

		for(elf32_word_t i=0; i<header->e_shnum; i++, section++)
		{
			if(i == header->e_shstrndx)
				continue;

			switch(section->sh_type)
			{
				case SHT_STRTAB:
					__io_kernelLibrary->strtab = (const char *)(((char *)header) + section->sh_offset);
					__io_kernelLibrary->strtabSize = section->sh_size;
					break;

				case SHT_SYMTAB:
					__io_kernelLibrary->symtab = (elf_sym_t *)(((char *)header) + section->sh_offset);
					__io_kernelLibrarySymbolCount = (section->sh_size / section->sh_entsize);
					break;

				default:
					break;
			}
		}

		// Initialize the kernel library stub and allocate enough space for the exported symbol section
		__io_kernelLibrary->name = __io_kernelLibrary->path = "firedrake";
		__io_kernelLibrary->relocBase = 0x0;
		__io_exportedSymbols = halloc(NULL, (sizeof(__io_exportedSymbolNames) / sizeof(char *)) * sizeof(elf_sym_t *));

		return (__io_kernelLibrary->strtab && __io_kernelLibrary->symtab && __io_exportedSymbols);
	}

	return true;
}
