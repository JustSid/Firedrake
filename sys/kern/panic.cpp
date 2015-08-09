//
//  panic.cpp
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

#include <machine/cpu.h>
#include <machine/interrupts/interrupts.h>
#include <libc/stdarg.h>
#include <libc/stdio.h>
#include <libc/backtrace.h>
#include <libio/core/IOArray.h>
#include <os/linker/LDModule.h>
#include <os/scheduler/task.h>
#include <os/scheduler/scheduler.h>
#include <machine/memory/heap.h>
#include "panic.h"
#include "kprintf.h"

#define kPanicMaxBacktraceFrames 20

namespace OS
{
	namespace LD
	{
		extern IO::Array *__GetAllModules();
	}
}

static char _panicScribbleArea[4096];
static bool _panic_initialized = false;
static IO::Array *_panicLoadedModules = nullptr;

bool grab_modules()
{
	if(!_panicLoadedModules)
		_panicLoadedModules = OS::LD::__GetAllModules();

	return (_panicLoadedModules != nullptr);
}

void dump_modules()
{
	if(grab_modules())
	{
		kputs("\nLoaded modules:\n");

		_panicLoadedModules->Enumerate<OS::LD::Module>([](OS::LD::Module *module, __unused size_t index, __unused bool &stop) {
			kprintf("  %s: %p - %p\n", module->GetPath(), module->GetMemory(), module->GetMemory() + (module->GetPages() * VM_PAGE_SIZE));
		});
	}
	else
	{
		kputs("\nCouldn't access kernel modules because it wasn't safe\n");
	}
}

void dump_frame(int index, void *tframe)
{
	uintptr_t frame = reinterpret_cast<uintptr_t >(tframe);
	OS::LD::Module *module = nullptr;

	if(grab_modules())
	{
		_panicLoadedModules->Enumerate<OS::LD::Module>([&](OS::LD::Module *temp, __unused size_t index, bool &stop) {

			if(temp->ContainsAddress(frame))
			{
				module = temp;
				stop = true;
			}

		});
	}


	kprintf("  (%2i) %08x", index, static_cast<uint32_t>(frame));

	if(frame >= IR_TRAMPOLINE_BEGIN && frame <= IR_TRAMPOLINE_BEGIN + (IR_TRAMPOLINE_PAGES * VM_PAGE_SIZE))
		kputs(" (trampoline)");

	if(module)
		kprintf(" (%s)", module->GetPath());

	kputs("\n");
}

void panic_die(const char *buffer)
{
	Sys::CPU *cpu = Sys::CPU::GetCurrentCPU();
	Sys::CPUState *state = cpu->GetLastState();

	kputs("\n\033[31mKernel Panic!\033[0m\n");
	kputs("Reason: \"");
	kputs(buffer);
	kputs("\"\n");

	OS::Thread *thread = OS::Scheduler::GetScheduler()->GetActiveThread();

	if(thread)
	{
		OS::Task *task = thread->GetTask();

		pid_t pid = task->GetPid();
		tid_t tid = thread->GetTid();

		const char *name = task->GetName() ? task->GetName()->GetCString() : nullptr;

		kprintf("Crashing CPU: \033[1;34m%i\033[0m, Task: #%d (%s), Thread: %d\n", cpu->GetID(), pid, name, tid);
	}
	else
	{
		kprintf("Crashing CPU: \033[1;34m%i\033[0m\n", cpu->GetID());
	}

	if(state)
	{
		kprintf("CPU State (interrupt vector \033[1;34m0x%x\033[0m):\n", state->interrupt);
		kprintf("  eax: \033[1;34m%08x\033[0m, ecx: \033[1;34m%08x\033[0m, edx: \033[1;34m%08x\033[0m, ebx: \033[1;34m%08x\033[0m\n", state->eax, state->ecx, state->edx, state->ebx);
		kprintf("  esp: \033[1;34m%08x\033[0m, ebp: \033[1;34m%08x\033[0m, esi: \033[1;34m%08x\033[0m, edi: \033[1;34m%08x\033[0m\n", state->esp, state->ebp, state->esi, state->edi);
		kprintf("  eip: \033[1;34m%08x\033[0m, eflags: \033[1;34m%08x\033[0m.\n", state->eip, state->eflags);
	}

	void *frames[kPanicMaxBacktraceFrames];

	if(thread && state)
	{
		kputs("\nBacktrace:\n");

		frames[0] = reinterpret_cast<void *>(state->eip);

		int num = backtrace_np(reinterpret_cast<void *>(state->ebp), frames + 1, kPanicMaxBacktraceFrames - 1) + 1;

		for(int i = 0; i < num; i ++)
			dump_frame((num - 1) - i, frames[i]);
	}

	{
		kputs("\nKernel backtrace:\n");

		int num = backtrace(frames, kPanicMaxBacktraceFrames);

		for(int i = 0; i < num; i ++)
			dump_frame((num - 1) - i, frames[i]);
	}

	dump_modules();
	kputs("\nCPU halt");
}
void panic(const char *reason, ...)
{
	if(_panic_initialized)
	{
		// Tear down the system properly to avoid other code to be executed
		Sys::DisableInterrupts();
		Sys::APIC::BroadcastIPI(0x39, false);
	}

	Sys::Heap::SwitchToPanicHeap();

	va_list args;
	va_start(args, reason);
	
	vsnprintf(_panicScribbleArea, 4096, reason, args);
	panic_die(_panicScribbleArea);

	va_end(args);

	while(1)
	{
		Sys::DisableInterrupts();
		Sys::CPUHalt();
	}
}

void panic_init()
{
	_panic_initialized = true;
}
