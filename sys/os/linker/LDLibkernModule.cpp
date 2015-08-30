//
//  LDLibkernModule.cpp
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2015 by Sidney Just
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
#include <kern/kalloc.h>
#include <kern/panic.h>
#include <libio/core/IOCatalogue.h>
#include <libio/core/IONull.h>
#include <libio/core/IOArray.h>
#include <libio/core/IORegistry.h>
#include <libio/hid/IOHIDKeyboardUtilities.h>
#include <os/scheduler/scheduler.h>
#include <machine/interrupts/interrupts.h>
#include "LDModule.h"
#include "LDService.h"

// -------------
// The libkern.so library is a bit of hack since it provides symbols from the kernel
// which are overriden hre to point to the real symbols in the kernel.
//
// It also contains stubs for __libkern_getIOCatalogue() and __libkern_getIONull()
// which bridge the custom built libio/core in the Firedrake kernel
// and the actual libio dynamic library.
// -------------

namespace OS
{
	namespace LD
	{
		void *__libio_getIOCatalogue()
		{
			return IO::Catalogue::GetSharedInstance();
		}
		void *__libio_getIONull()
		{
			return IO::Catalogue::GetSharedInstance();
		}
		void *__libio_getIORootRegistry()
		{
			return IO::RegistryEntry::GetRootEntry();
		}


		void __libkern_threadEntry(void (*entry)(void *), void *argument)
		{
			entry(argument);

			Scheduler *scheduler = Scheduler::GetScheduler();
			Thread *thread = scheduler->GetActiveThread();

			scheduler->BlockThread(thread);
			scheduler->RemoveThread(thread);

			scheduler->GetActiveTask()->MarkThreadExit(thread);

			while(1)
			{}
		}


		void thread_yield()
		{
			Scheduler *scheduler = Scheduler::GetScheduler();
			scheduler->YieldThread(scheduler->GetActiveThread());
		}

		void thread_create(void (*entry)(void *), void *argument)
		{
			Task *task = Scheduler::GetScheduler()->GetActiveTask();

			IO::Number *entryNum = IO::Number::Alloc()->InitWithUint32(reinterpret_cast<uint32_t>(entry));
			IO::Number *argNum = IO::Number::Alloc()->InitWithUint32(reinterpret_cast<uint32_t>(argument));

			IO::Array *parameters = IO::Array::Alloc()->Init();
			parameters->AddObject(entryNum);
			parameters->AddObject(argNum);

			entryNum->Release();
			argNum->Release();

			task->AttachThread(reinterpret_cast<Thread::Entry >(&__libkern_threadEntry), Thread::PriorityClassKernel, 0, parameters);

			parameters->Release();
		}


		typedef void (*InterruptHandler)(void *argument, uint8_t vector);

		struct InterruptEntry
		{
			InterruptHandler handler;
			void *argument;
		};

		static Mutex __interruptLock;
		static InterruptEntry __interruptEntries[255];

		uint32_t __libkern_interruptHandler(uint32_t esp, __unused Sys::CPU *cpu)
		{
			Sys::CPUState *state = reinterpret_cast<Sys::CPUState *>(esp);
			InterruptEntry *entry = __interruptEntries + state->interrupt;

			entry->handler(entry->argument, static_cast<uint8_t>(state->interrupt));

			return esp;
		}

		void register_interrupt(uint8_t vector, void *argument, InterruptHandler handler)
		{
			__interruptLock.Lock(Mutex::Mode::NoInterrupts);

			InterruptEntry *entry = __interruptEntries + vector;
			entry->argument = argument;
			entry->handler = handler;

			__interruptLock.Unlock();

			if(vector < 0x30)
				Sys::APIC::MaskInterrupt(vector, (handler == nullptr));

			Sys::SetInterruptHandler(vector, &__libkern_interruptHandler);
		}

		void __libkern_dispatchKeyboardEvent(IO::KeyboardEvent *event)
		{
			if(event->IsKeyDown())
			{
				kprintf("%c", event->GetCharacter());

				switch(event->GetKeyCode())
				{
					case IO::KeyCodeArrowUp:
						kprintf("\033[1A");
						break;
					case IO::KeyCodeArrowDown:
						kprintf("\033[1B");
						break;
					case IO::KeyCodeArrowLeft:
						kprintf("\033[1D");
						break;
					case IO::KeyCodeArrowRight:
						kprintf("\033[1C");
						break;

					default:
						break;
				}
			}
		}


		ipc_return_t ipc_write(ipc_header_t *header)
		{
			IPC::Space *space = IPC::Space::GetKernelSpace();
			IPC::Message *message = IPC::Message::Alloc()->Init(header);

			space->Lock();
			KernReturn<void> result = space->Write(message);
			space->Unlock();

			message->Release();

			return result.IsValid() ? KERN_SUCCESS : result.GetError().GetCode();
		}
		ipc_return_t ipc_read(ipc_header_t *header)
		{
			IPC::Space *space = IPC::Space::GetKernelSpace();
			IPC::Message *message = IPC::Message::Alloc()->Init(header);

			space->Lock();
			KernReturn<void> result = space->Read(message);
			space->Unlock();

			message->Release();

			return result.IsValid() ? KERN_SUCCESS : result.GetError().GetCode();
		}

		ipc_return_t ipc_allocate_port(ipc_port_t *port)
		{
			IPC::Space *space = IPC::Space::GetKernelSpace();
			space->Lock();

			KernReturn<IPC::Port *> actual = space->AllocateReceivePort();
			if(!actual.IsValid())
			{
				space->Unlock();
				return actual.GetError().GetCode();
			}

			*port = actual->GetName();
			space->Unlock();

			return KERN_SUCCESS;
		}

		ipc_return_t ipc_port_deallocate(ipc_port_t port)
		{
			IPC::Space *space = IPC::Space::GetKernelSpace();
			space->Lock();

			IPC::Port *actual = space->GetPortWithName(port);
			if(!actual)
			{
				space->Unlock();
				return KERN_INVALID_ARGUMENT;
			}

			space->DeallocatePort(actual);
			space->Unlock();

			return KERN_SUCCESS;
		}

		ipc_port_t ipc_get_special_port(int port)
		{
			switch(port)
			{
				case 0:
					return GetKernelPort()->GetName();
			}

			return IPC_PORT_NULL;
		}


#define ELF_SYMBOL_STUB(function) \
		{ #function, { kModuleSymbolStubName, (elf32_address_t)function, 0, 0, 0, 0 } }

		struct LibKernSymbol
		{
			const char *name;
			elf_sym_t symbol;
		};

		LibKernSymbol __libkernModuleSymbols[] =
			{
				ELF_SYMBOL_STUB(panic),
				ELF_SYMBOL_STUB(kprintf),
				ELF_SYMBOL_STUB(kputs),
				ELF_SYMBOL_STUB(knputs),
				ELF_SYMBOL_STUB(kalloc),
				ELF_SYMBOL_STUB(kfree),
				ELF_SYMBOL_STUB(__libio_getIOCatalogue),
				ELF_SYMBOL_STUB(__libio_getIONull),
				ELF_SYMBOL_STUB(__libio_getIORootRegistry),
				ELF_SYMBOL_STUB(thread_yield),
				ELF_SYMBOL_STUB(thread_create),
				ELF_SYMBOL_STUB(register_interrupt),
				ELF_SYMBOL_STUB(__libkern_dispatchKeyboardEvent),
				ELF_SYMBOL_STUB(ipc_write),
				ELF_SYMBOL_STUB(ipc_read),
				ELF_SYMBOL_STUB(ipc_allocate_port),
				ELF_SYMBOL_STUB(ipc_port_deallocate),
				ELF_SYMBOL_STUB(ipc_get_special_port)
			};

		elf_sym_t *LibkernModule::GetSymbolWithName(const char *name)
		{
			for(size_t i = 0; i < sizeof(__libkernModuleSymbols) / sizeof(LibKernSymbol); i ++)
			{
				if(strcmp(name, __libkernModuleSymbols[i].name) == 0)
				{
					return &__libkernModuleSymbols[i].symbol;
				}
			}

			return Module::GetSymbolWithName(name);
		}

		void *LibkernModule::GetSymbolAddressWithName(const char *name)
		{
			elf_sym_t *symbol = GetSymbolWithName(name);
			if(symbol)
			{
				if(symbol->st_name == kModuleSymbolStubName)
					return reinterpret_cast<void *>(symbol->st_value);

				return reinterpret_cast<void *>(GetRelocationBase() + symbol->st_value);
			}

			return nullptr;
		}
	}
}
