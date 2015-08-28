//
//  LDModule.h
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

#ifndef _LDMODULE_H_
#define _LDMODULE_H_

#include <prefix.h>
#include <libio/core/IOObject.h>
#include <libio/core/IOArray.h>
#include <libio/core/IOString.h>
#include <os/loader/elf.h>

#define kModuleSymbolStubName 0xdeadbeef

namespace OS
{
	namespace LD
	{
		class Module : public IO::Object
		{
		public:
			enum class Type
			{
				Library, // Simple .so library
				Extension // Kernel extension
			};

			typedef bool (*StartFunction)(void *token);
			typedef void (*StopFunction)(void *token);

			KernReturn<Module *> InitWithPath(const char *path);

			KernReturn<void> FinishLoading();

			bool Start();
			void Stop();

			const char *GetName() const { return _name; }
			const char *GetPath() const { return _path; }

			bool ContainsAddress(vm_address_t address) const;

			Type GetType() const { return _type; }
			size_t GetRelocationBase() const { return _relocationBase; }

			virtual elf_sym_t *GetSymbolWithName(const char *name);
			virtual void *GetSymbolAddressWithName(const char *name);

			size_t GetPages() const { return _pages; }
			const uint8_t *GetMemory() const { return reinterpret_cast<uint8_t *>(_memory); }

		protected:
			Module *Init(Type type);

		private:
			void Dealloc();

			KernReturn<void> Load(void *data);
			void DigestDynamicSection();

			KernReturn<void> ResolveDependencies();

			KernReturn<void> RelocatePLT();
			KernReturn<void> RelocateGOT();
			void RunInitFunctions();

			void *LookupAddress(const char *name, Module *&outLib);

			elf_dyn_t *_dynamic;

			const char *_strTable;
			size_t _strTableSize;
			elf_sym_t *_symTable;

			elf_rel_t *_rel;
			elf_rel_t *_relLimit;
			elf_rel_t *_pltRel;
			elf_rel_t *_pltRelLimit;
			elf_rela_t *_rela;
			elf_rela_t *_relaLimit;

			uint32_t *_hashTable;
			uint32_t *_buckets;
			uint32_t *_chains;
			uint32_t _bucketCount;
			uint32_t _chainsCount;

			uintptr_t *_initArray;
			size_t _initArrayCount;

			size_t _relocationBase;
			size_t _pages;
			void *_memory;

			IO::Array *_dependencies;
			char *_path;
			char *_name;

			Type _type;
			StartFunction _start;
			StopFunction _stop;

			IODeclareMeta(Module)
		};

		class LibkernModule : public Module
		{
		public:
			elf_sym_t *GetSymbolWithName(const char *name) override;
			void *GetSymbolAddressWithName(const char *name) override;

		private:
			IODeclareMeta(LibkernModule)

		};
	}
}


#endif /* _LDMODULE_H_ */
