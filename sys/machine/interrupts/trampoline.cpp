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
	struct TrampolineMap
	{
		uint8_t buffer[VM_PAGE_SIZE]; // Contains the trampoline area executable code
		uint8_t buffer2[VM_PAGE_SIZE]; // Contains auxiliary data, for now just the address of the kernel page directory
		Trampoline trampoline[CONFIG_MAX_CPUS];

		char padding[(VM_PAGE_COUNT((sizeof(Trampoline) * CONFIG_MAX_CPUS)) * VM_PAGE_SIZE) - (sizeof(Trampoline) * CONFIG_MAX_CPUS)]; // Pad to the next page
		CPUData trampolineData[CONFIG_MAX_CPUS];
	};
	
	TrampolineMap *_map = nullptr;
	uintptr_t _physicalTrampoline = 0x0;

	KernReturn<void> TrampolineInit()
	{
		assert(sizeof(TrampolineMap) <= IR_TRAMPOLINE_PAGES * VM_PAGE_SIZE);

		KernReturn<vm_address_t> vaddress;
		KernReturn<uintptr_t> paddress;

		// Allocate space for the trampoline map
		paddress = PM::Alloc(IR_TRAMPOLINE_PAGES);
		if(paddress.IsValid() == false)
		{
			kprintf("failed to allocate physical trampoline area");
			return paddress.GetError();
		}

		vaddress = VM::Directory::GetKernelDirectory()->AllocLimit(paddress, IR_TRAMPOLINE_BEGIN, VM::kUpperLimit, IR_TRAMPOLINE_PAGES, kVMFlagsKernel);
		if(!vaddress.IsValid() || vaddress != IR_TRAMPOLINE_BEGIN)
		{
			kprintf("failed to allocate trampoline area");
			return vaddress.GetError();
		}
		
		_map = reinterpret_cast<TrampolineMap *>(IR_TRAMPOLINE_BEGIN);
		_physicalTrampoline = paddress;

		// Fix up the idt section
		uintptr_t idtBegin = reinterpret_cast<uintptr_t>(&idt_begin);
		uintptr_t idtEnd   = reinterpret_cast<uintptr_t>(&idt_end);

		size_t size = static_cast<size_t>(idtEnd - idtBegin);
		assert(size <= VM_PAGE_SIZE);

		memcpy(_map->buffer, &idt_begin, idtEnd - idtBegin);

		// Write the physical page directory of the kernel
		uint32_t **directory = reinterpret_cast<uint32_t **>(&_map->buffer2);
		*directory = VM::Directory::GetKernelDirectory()->GetPhysicalDirectory();

		return TrampolineInitCPU();
	}

	KernReturn<void> TrampolineInitCPU()
	{
		CPU *cpu = CPU::GetCurrentCPU();
		Trampoline *trampoline = &_map->trampoline[cpu->GetID()];
		CPUData *trampolineData = &_map->trampolineData[cpu->GetID()];

		VM::Directory *directory = VM::Directory::GetKernelDirectory();

		trampoline->pageDirectory = directory->GetPhysicalDirectory();
		cpu->SetTrampoline(trampoline);

		uintptr_t idtBegin = reinterpret_cast<uintptr_t>(&idt_begin);
		IDTInit(trampoline->idt, IR_TRAMPOLINE_BEGIN - idtBegin);
		GDTInit(trampoline->gdt, &trampoline->tss, trampolineData);

		trampolineData->cpuID = cpu->GetID();

		// Hacky hackery hack
		uint32_t *esp = Alloc<uint32_t>(directory, 1, kVMFlagsKernel);
		trampoline->tss.esp0 = reinterpret_cast<uint32_t>(esp) + (VM_PAGE_SIZE - sizeof(Sys::CPUState));
		trampoline->tss.ss0  = 0x10;

		return ErrorNone;
	}

	KernReturn<void> TrampolineMapIntoDirectory(VM::Directory *directory)
	{
		KernReturn<vm_address_t> vaddress = directory->AllocLimit(_physicalTrampoline, IR_TRAMPOLINE_BEGIN, VM::kUpperLimit, IR_TRAMPOLINE_PAGES, kVMFlagsKernel);
		if(!vaddress.IsValid() || vaddress != IR_TRAMPOLINE_BEGIN)
			return vaddress.GetError();

		size_t offset = offsetof(TrampolineMap, trampolineData);
		offset += CPU::GetCurrentCPU()->GetID() * sizeof(CPUData);

		directory->MapPageRange(_physicalTrampoline + offset, IR_TRAMPOLINE_BEGIN + offset, 2, kVMFlagsUserlandR);

		return ErrorNone;
	}
}
