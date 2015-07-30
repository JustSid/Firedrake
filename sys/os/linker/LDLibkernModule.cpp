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
#include <libio/IOCatalogue.h>
#include <libio/IONull.h>
#include "LDModule.h"

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
		void *__libkern_getIOCatalogue()
		{
			return IO::Catalogue::GetSharedInstance();
		}
		void *__libkern_getIONull()
		{
			return IO::Catalogue::GetSharedInstance();
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
				ELF_SYMBOL_STUB(__libkern_getIOCatalogue),
				ELF_SYMBOL_STUB(__libkern_getIONull)
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
