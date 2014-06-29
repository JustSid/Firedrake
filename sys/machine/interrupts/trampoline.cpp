//
//  trampoline.cpp
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

#include <kern/kprintf.h>
#include <libc/string.h>
#include <libc/assert.h>
#include <machine/gdt.h>
#include <machine/memory/memory.h>
#include <machine/cme.h>
#include <machine/cpu.h>
#include "trampoline.h"
#include "interrupts.h"

extern "C" uintptr_t idt_begin;
extern "C" uintptr_t idt_end;

namespace Sys
{
	struct Map
	{
		uint8_t buffer[VM_PAGE_SIZE];
		Trampoline trampoline[CONFIG_MAX_CPUS];
	};
	Map *_map = nullptr;

	kern_return_t TrampolineInit()
	{
		assert(sizeof(Map) <= IR_TRAMPOLINE_PAGES * VM_PAGE_SIZE);

		kern_return_t result;
		vm_address_t vaddress;
		uintptr_t paddress;

		// Allocate space for the trampoline map
		result = PM::Alloc(paddress, IR_TRAMPOLINE_PAGES);
		if(result != KERN_SUCCESS)
		{
			kprintf("failed to allocate physical trampoline area");
			return result;
		}

		result = VM::Directory::GetKernelDirectory()->AllocLimit(vaddress, paddress, IR_TRAMPOLINE_BEGIN, VM::kUpperLimit, IR_TRAMPOLINE_PAGES, kVMFlagsKernel);
		if(result != KERN_SUCCESS || vaddress != IR_TRAMPOLINE_BEGIN)
		{
			kprintf("failed to allocate trampoline area");
			return result;
		}
		
		_map = reinterpret_cast<Map *>(IR_TRAMPOLINE_BEGIN);

		// Fix up the idt section
		uintptr_t idtBegin = reinterpret_cast<uintptr_t>(&idt_begin);
		uintptr_t idtEnd   = reinterpret_cast<uintptr_t>(&idt_end);

		size_t size = static_cast<size_t>(idtEnd - idtBegin);
		assert(size <= VM_PAGE_SIZE);

		memcpy(_map->buffer, &idt_begin, idtEnd - idtBegin);
		return TrampolineInitCPU();
	}

	kern_return_t TrampolineInitCPU()
	{
		CPU *cpu = CPU::GetCurrentCPU();
		Trampoline *trampoline = &_map->trampoline[cpu->GetID()];

		VM::Directory *directory = VM::Directory::GetKernelDirectory();

		trampoline->pageDirectory = directory->GetPhysicalDirectory();
		cpu->SetTrampoline(trampoline);

		uintptr_t idtBegin = reinterpret_cast<uintptr_t>(&idt_begin);
		IDTInit(trampoline->idt, IR_TRAMPOLINE_BEGIN - idtBegin);
		GDTInit(trampoline->gdt, &trampoline->tss);

		// Hacky hackery hack
		uint32_t *esp = Alloc<uint32_t>(directory, 1, kVMFlagsKernel);
		trampoline->tss.esp0 = reinterpret_cast<uint32_t>(esp) + (VM_PAGE_SIZE - sizeof(Sys::CPUState));
		trampoline->tss.ss0  = 0x10;

		return KERN_SUCCESS;
	}
}
