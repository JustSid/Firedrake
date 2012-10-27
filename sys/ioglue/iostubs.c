//
//  iostubs.c
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

#include <bootstrap/multiboot.h>
#include <interrupts/interrupts.h>
#include <scheduler/scheduler.h>
#include <memory/memory.h>
#include <system/helper.h>
#include <system/syslog.h>
#include <system/cpu.h>
#include <system/panic.h>
#include <libc/string.h>

#include "iostubs.h"
#include "iostore.h"

// Actual stub implementation

heap_t *__io_heap = NULL;
spinlock_t __io_lazy_lock = SPINLOCK_INIT; // Lock used todo lazy allocation

void __IOPrimitiveLog(char *message)
{
	syslog(LOG_INFO, "%s\n", message);
}

uint32_t __IOPrimitiveThreadCreate(void *entry, void *arg)
{
	process_t *process = process_getCurrentProcess();
	thread_t *thread = thread_create(process, (thread_entry_t)entry, 4096, 1, arg);

	return thread->id;
}

uint32_t __IOPrimitiveThreadID()
{
	thread_t *thread = thread_getCurrentThread();
	return thread->id;
}

void *__IOPrimitiveThreadAccessArgument(size_t index)
{
	thread_t *thread = thread_getCurrentThread();
	return (void *)thread->arguments[index];
}

void __IOPrimitiveThreadDied()
{
	sd_threadExit();
}

void *IOMalloc(size_t size)
{
	spinlock_lock(&__io_lazy_lock);

	if(__io_heap == NULL)
	{
		__io_heap = heap_create((1024 * 1024) * 5, vm_getKernelDirectory(), kHeapFlagSecure | kHeapFlagUntrusted);
		if(!__io_heap)
			panic("Couldn't create heap for libio!");
	}

	spinlock_unlock(&__io_lazy_lock);


	void *mem = halloc(__io_heap, size);
	return mem;
}

void IOFree(void *mem)
{
	hfree(__io_heap, mem);
}


spinlock_t __io_interrupt_lock = SPINLOCK_INIT;

typedef void (*IOServiceInterruptHandler)(void *target, uint32_t irq, void *context);
typedef struct
{
	IOServiceInterruptHandler handler;
	void *target;
	void *context;
} IOServiceInterruptHandlerContext;

IOServiceInterruptHandlerContext *__io_interruptHandler[IDT_ENTRIES];

uint32_t io_dispatchInterrupt(uint32_t esp)
{
	cpu_state_t *state = (cpu_state_t *)esp;
	IOServiceInterruptHandlerContext *context = __io_interruptHandler[state->interrupt];

	if(context)
	{
		IOServiceInterruptHandler handler = context->handler;
		handler(context->target, state->interrupt, context->context);
	}

	return esp;
}

IOReturn __IOServiceTryRegisterIRQ(void *target, uint32_t irq, void *src, IOServiceInterruptHandler handler)
{
	spinlock_lock(&__io_interrupt_lock);

	IOServiceInterruptHandlerContext *context = __io_interruptHandler[irq];
	if(context)
	{
		if(handler)
		{
			spinlock_unlock(&__io_interrupt_lock);
			return kIOReturnInterruptTaken;
		}

		hfree(NULL, context);
		__io_interruptHandler[irq] = NULL;

		spinlock_unlock(&__io_interrupt_lock);
		return kIOReturnSuccess;
	}


	context = halloc(NULL, sizeof(IOServiceInterruptHandlerContext));
	context->target = target;
	context->handler = handler;
	context->context = src;

	__io_interruptHandler[irq] = context;
	ir_setInterruptHandler(io_dispatchInterrupt, irq);

	spinlock_unlock(&__io_interrupt_lock);
	return kIOReturnSuccess;
}



// Symbol lookup

// List of exported symbols from the kernel
// The kernel will only allow libraries to link the symbols in this list
const char *__io_exportedSymbolNames[] = {
	"panic",
	"__IOPrimitiveLog",
	"__IOPrimitiveThreadCreate",
	"__IOPrimitiveThreadID",
	"__IOPrimitiveThreadAccessArgument",
	"__IOPrimitiveThreadDied",
	"__IOServiceTryRegisterIRQ",
	"IOMalloc",
	"IOFree"
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
		struct multiboot_module_s *module = sys_multibootModuleWithName("firedrake");
		if(!module)
			return false;

		elf_header_t *header = (elf_header_t *)module->start;
		elf_section_header_t *section = (elf_section_header_t *)(((char *)header) + header->e_shoff);

		for(elf32_word_t i=0; i<header->e_shnum; i++, section++)
		{
			if(section->sh_type == SHT_STRTAB && section->sh_flags == 0 && i != header->e_shstrndx)
			{
				__io_kernelLibrary->strtab = (const char *)(((char *)header) + section->sh_offset);
				__io_kernelLibrary->strtabSize = section->sh_size;
				continue;
			}

			if(section->sh_type == SHT_SYMTAB)
			{
				__io_kernelLibrary->symtab = (elf_sym_t *)(((char *)header) + section->sh_offset);
				__io_kernelLibrarySymbolCount = (section->sh_size / section->sh_entsize);
			}
		}

		size_t exportedSymbolCount = sizeof(__io_exportedSymbolNames) / sizeof(char *);

		__io_kernelLibrary->relocBase = 0x0;
		__io_exportedSymbols = halloc(NULL, exportedSymbolCount * sizeof(elf_sym_t *));

		return (__io_kernelLibrary->strtab && __io_kernelLibrary->symtab && __io_exportedSymbols);
	}

	return true;
}

