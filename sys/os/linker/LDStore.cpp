//
//  LDStore.cpp
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

#include <libio/IODictionary.h>
#include <os/locks/mutex.h>
#include "LDStore.h"

namespace OS
{
	namespace LD
	{
		static Mutex _moduleLock;
		static IO::Dictionary *_moduleStore;

		KernReturn<void> __AddModule(Module *module)
		{
			// No need to copy the name as the key is bound to the lifetime of the object in the dictionary
			IO::String *lookup = IO::String::Alloc()->InitWithCString(module->GetName(), false);
			if(_moduleStore->GetObjectForKey(lookup))
				panic("__AddModule() called with an already used module name (%s)", module->GetName());

			_moduleStore->SetObjectForKey(module, lookup);
			lookup->Release();

			KernReturn<void> result = module->FinishLoading();
			if(!result.IsValid())
			{
				_moduleStore->RemoveObjectForKey(lookup);
				return result.GetError();
			}

			// TODO: Remove this test line
			if(module->GetType() == Module::Type::Extension)
			{
				module->Start();
			}

			return ErrorNone;
		}

		KernReturn<void> AddModule(Module *module)
		{
			_moduleLock.Lock(Mutex::Mode::NoInterrupts);
			KernReturn<void> result = __AddModule(module);
			_moduleLock.Unlock();

			return result;
		}




		Module *__GetModuleWithNameNoLockPrivate(const char *name, bool loadIfNeeded)
		{
			if(strncmp(name, "/slib/", 6) == 0)
				name += 6; // Skip the /slib/ prefix

			IO::String *lookup = IO::String::Alloc()->InitWithCString(name, false);
			Module *result = _moduleStore->GetObjectForKey<Module>(lookup);
			lookup->Release();

			if(!result && loadIfNeeded)
			{
				char buffer[512];
				strcpy(buffer, "/slib/");
				strlcpy(buffer + strlen(buffer), name, 512 - strlen(buffer));

				KernReturn<Module *> module = Module::Alloc()->InitWithPath(buffer);
				if(!module.IsValid())
				{
					kprintf("Failed to load %s\n", buffer);
					return nullptr;
				}

				result = module.Get();
				__AddModule(result);
			}

			return result;
		}

		IO::StrongRef<Module> GetModuleWithName(const char *name, bool loadIfNeeded)
		{
			_moduleLock.Lock(Mutex::Mode::NoInterrupts);
			IO::StrongRef<Module> result = __GetModuleWithNameNoLockPrivate(name, loadIfNeeded);
			_moduleLock.Unlock();

			return result;
		}

		IO::StrongRef<Module> GetModuleWithAddress(vm_address_t address)
		{
			_moduleLock.Lock(Mutex::Mode::NoInterrupts);

			IO::StrongRef<Module> result(nullptr);

			_moduleStore->Enumerate<IO::String, Module>([&](IO::String *name, Module *module, bool &stop) {

				vm_address_t vlimit = reinterpret_cast<vm_address_t>(module->GetMemory() + (module->GetPages() * VM_PAGE_SIZE));
				if(address >= reinterpret_cast<vm_address_t>(module->GetMemory()) && address <= vlimit)
				{
					result = module;
					stop = true;
				}

			});

			_moduleLock.Unlock();

			return result;
		}

		IO::StrongRef<Module> LoadModuleWithName(const char *name)
		{
			return GetModuleWithName(name, true);
		}
	}

	KernReturn<void> LDInit()
	{
		LD::_moduleStore = IO::Dictionary::Alloc()->Init();
		if(!LD::_moduleStore)
			return Error(KERN_NO_MEMORY);

		KernReturn<LD::Module *> module = LD::LibkernModule::Alloc()->InitWithPath("/slib/libkern.so");
		if(!module.IsValid())
			return module.GetError();

		LD::AddModule(module);

		IO::StrongRef<LD::Module> libio = LD::LoadModuleWithName("libio.so");
		if(!libio)
			return Error(KERN_RESOURCES_MISSING);

		LD::LoadModuleWithName("libtest.so");

		return ErrorNone;
	}
}
