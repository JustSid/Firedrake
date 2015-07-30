//
//  LDModule.cpp
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

#include <vfs/vfs.h>
#include <os/loader/elf.h>
#include "LDModule.h"
#include "LDStore.h"

namespace OS
{
	namespace LD
	{
		IODefineMeta(Module, IO::Object)
		IODefineScopendMeta(Module, Dependency, IO::Object)
		IODefineMeta(LibkernModule, Module)

		extern Module *__GetModuleWithNameNoLockPrivate(const char *name, bool loadIfNeeded);

		Module* Module::Init(Type type)
		{
			if(!IO::Object::Init())
				return nullptr;

			// Initialization
			_dynamic = nullptr;
			_strTable = nullptr;
			_symTable = nullptr;
			_hashTable = nullptr;
			_buckets = nullptr;
			_chains = nullptr;
			_initArray = nullptr;

			_rel = _relLimit = nullptr;
			_pltRel = _pltRelLimit = nullptr;
			_rela = _relaLimit = nullptr;

			_bucketCount = 0;
			_chainsCount = 0;
			_initArrayCount = 0;
			_relocationBase = 0;

			_dependencies = nullptr;
			_name = nullptr;
			_path = nullptr;
			_memory = nullptr;

			_type = type;

			return this;
		}

		KernReturn<Module *> Module::InitWithPath(const char *path)
		{
			if(!Init(Type::Library))
				return nullptr;

			// Read the binary from the filesystem
			KernReturn<void> status;
			KernReturn<int> fd;
			VFS::Context *context = VFS::Context::GetKernelContext();

			if((fd = VFS::Open(context, path, O_RDONLY)).IsValid() == false)
				return fd.GetError();

			KernReturn<void> result;
			KernReturn<off_t> size;
			size_t left;

			uint8_t *temp;
			uint8_t *buffer = nullptr;

			// Get the size of the file and allocate enough storage
			if((size = VFS::Seek(context, fd, 0, SEEK_END)).IsValid() == false)
			{
				status = size.GetError();
				goto exit;
			}

			left   = size;
			buffer = new uint8_t[size];

			if(!buffer)
			{
				status = Error(KERN_NO_MEMORY);
				goto exit;
			}

			if(!VFS::Seek(context, fd, 0, SEEK_SET).IsValid())
			{
				status = Error(KERN_FAILURE);
				goto exit;
			}

			temp = buffer;

			// Read the file
			while(left > 0)
			{
				KernReturn<size_t> read = VFS::Read(context, fd, temp, left);

				if(!read.IsValid())
				{
					status = read.GetError();
					goto exit;
				}

				temp += read;
				left -= read;
			}

			result = Load(buffer);
			status = result.IsValid() ? ErrorNone : result.GetError();

			if(result.IsValid())
			{
				// Copy the name
				size_t pathLength = strlen(path);

				_path = new char[pathLength + 1];
				strlcpy(_path, path, pathLength + 1);

				_name = _path + pathLength;

				while(_name > _path && *_name != '/')
					_name --;

				if(*_name == '/')
					_name ++;
			}

		exit:
			VFS::Close(context, fd);
			delete[] buffer;

			if(!status.IsValid())
				return status.GetError();

			return this;
		}

		void Module::Dealloc()
		{
			delete[] _path;
			IO::SafeRelease(_dependencies);

			if(_memory)
				Sys::Free(_memory, Sys::VM::Directory::GetKernelDirectory(), _pages);

			IO::Object::Dealloc();
		}

		KernReturn<void> Module::Load(void *data)
		{
			elf_header_t *header = static_cast<elf_header_t *>(data);

			if(strncmp((const char *)header->e_ident, ELF_MAGIC, strlen(ELF_MAGIC)) != 0 || header->e_type != ET_DYN)
				return Error(KERN_INVALID_ARGUMENT, "Invalid elf header");

			uint8_t *buffer = static_cast<uint8_t  *>(data);

			// Parse the program header
			elf_program_header_t *programHeader = reinterpret_cast<elf_program_header_t *>(buffer + header->e_phoff);
			elf_program_header_t *segments[8];

			vm_address_t minAddress = -1;
			vm_address_t maxAddress = 0;

			size_t segmentCount = 0;

			// Calculate the needed size
			for(size_t i = 0; i < header->e_phnum; i ++)
			{
				elf_program_header_t *program = &programHeader[i];

				if(program->p_type == PT_LOAD)
				{
					minAddress = std::min(program->p_paddr, static_cast<elf32_address_t >(minAddress));
					maxAddress = std::max(program->p_paddr + program->p_memsz, static_cast<elf32_address_t >(maxAddress));

					segments[segmentCount ++] = program;
				}

				if(program->p_type == PT_DYNAMIC)
					_dynamic = reinterpret_cast<elf_dyn_t *>(program->p_vaddr);
			}

			_pages = VM_PAGE_COUNT(maxAddress - minAddress);
			_memory = Sys::Alloc(Sys::VM::Directory::GetKernelDirectory(), _pages, kVMFlagsKernel);

			// Copy the library over
			memset(_memory, 0, _pages * VM_PAGE_SIZE);

			uint8_t *target = static_cast<uint8_t *>(_memory);

			for(size_t i = 0; i < segmentCount; i ++)
			{
				elf_program_header_t *program = segments[i];
				memcpy(&target[program->p_vaddr - minAddress], &buffer[program->p_offset], program->p_filesz);
			}

			_relocationBase = reinterpret_cast<uintptr_t>(_memory) - minAddress;

			if(_dynamic)
			{
				_dynamic = reinterpret_cast<elf_dyn_t *>(reinterpret_cast<uint8_t *>(_dynamic) + _relocationBase);
				DigestDynamicSection();
			}

			return ErrorNone;
		}

		void Module::DigestDynamicSection()
		{
			bool usePLTRel  = false;
			bool usePLTRela = false;

			size_t relSize = 0;
			size_t relaSize = 0;

			elf32_address_t pltRel = 0;
			size_t pltRelSize = 0;

			for(elf_dyn_t *dyn = _dynamic; dyn->d_tag != DT_NULL; dyn ++)
			{
				switch(dyn->d_tag)
				{
					case DT_NEEDED:
					{
						if(!_dependencies)
							_dependencies = IO::Array::Alloc()->Init();

						Dependency *dependency = Dependency::Alloc()->Init(dyn->d_un.d_val);
						_dependencies->AddObject(dependency);
						dependency->Release();

						break;
					}

					case DT_REL:
						_rel = (elf_rel_t *)(_relocationBase + dyn->d_un.d_ptr);
						break;

					case DT_RELSZ:
						relSize = dyn->d_un.d_val;
						break;

					case DT_JMPREL:
						pltRel = dyn->d_un.d_ptr;
						break;

					case DT_PLTRELSZ:
						pltRelSize = dyn->d_un.d_val;
						break;

					case DT_RELENT:
						assert(dyn->d_un.d_val == sizeof(elf_rel_t));
						break;

					case DT_RELA:
						_rela = (elf_rela_t *)(_relocationBase + dyn->d_un.d_ptr);
						break;

					case DT_RELASZ:
						relaSize = dyn->d_un.d_val;
						break;

					case DT_STRTAB:
						_strTable = (const char *)(_relocationBase + dyn->d_un.d_ptr);
						break;

					case DT_STRSZ:
						_strTableSize = dyn->d_un.d_val;
						break;

					case DT_SYMTAB:
						_symTable = (elf_sym_t *)(_relocationBase + dyn->d_un.d_ptr);
						break;

					case DT_SYMENT:
						assert(dyn->d_un.d_val == sizeof(elf_sym_t));
						break;

					case DT_HASH:
						_hashTable = (uint32_t *)(_relocationBase + dyn->d_un.d_ptr);

						_bucketCount = _hashTable[0];
						_chainsCount = _hashTable[1];

						_buckets  = _hashTable + 2;
						_chains   = _buckets + _bucketCount;
						break;

					case DT_PLTREL:
						usePLTRel  = (dyn->d_un.d_val == DT_REL);
						usePLTRela = (dyn->d_un.d_val == DT_RELA);
						break;

					case DT_INIT_ARRAY:
						_initArray = (uintptr_t *)(_relocationBase + dyn->d_un.d_ptr);
						break;

					case DT_INIT_ARRAYSZ:
						_initArrayCount = (dyn->d_un.d_val / sizeof(uintptr_t));
						break;

					default:
						break;
				}
			}

			_relLimit = reinterpret_cast<elf_rel_t *>(reinterpret_cast<uint8_t *>(_rel) + relSize);
			_relaLimit = reinterpret_cast<elf_rela_t *>(reinterpret_cast<uint8_t *>(_rela) + relaSize);

			if(usePLTRel)
			{
				_pltRel = reinterpret_cast<elf_rel_t *>(_relocationBase + pltRel);
				_pltRelLimit = reinterpret_cast<elf_rel_t *>(_relocationBase + pltRel + pltRelSize);
			}
			else if(usePLTRela)
			{
				panic("PLTRela not yet implemented");
			}
		}

		KernReturn<void> Module::FinishLoading()
		{
			KernReturn<void> result;

			if((result = ResolveDependencies()).IsValid() == false)
				return result.GetError();

			if((result = RelocatePLT()).IsValid() == false)
				return result.GetError();

			if((result = RelocateGOT()).IsValid() == false)
				return result.GetError();

			_start = reinterpret_cast<StartFunction>(GetSymbolAddressWithName("_kern_start"));
			_stop = reinterpret_cast<StopFunction>(GetSymbolAddressWithName("_kern_stop"));

			_type = (_start && _stop) ? Type::Extension : Type::Library;

			return ErrorNone;
		}

		KernReturn<void> Module::ResolveDependencies()
		{
			KernReturn<void> result = ErrorNone;

			if(!_dependencies)
				return result;

			_dependencies->Enumerate<Dependency>([&](Dependency *dependency, size_t index, bool &stop) {

				const char *name = _strTable + dependency->GetName();
				Module *module = __GetModuleWithNameNoLockPrivate(name, true);

				if(!module)
				{
					result = Error(KERN_RESOURCES_MISSING, "Could not find module with name %s", name);
					stop = true;

					return;
				}

				dependency->SetDependency(module);

			});

			return result;
		}

		KernReturn<void> Module::RelocatePLT()
		{
			for(elf_rel_t *rel = _pltRel; rel < _pltRelLimit; rel ++)
			{
				elf32_address_t *address = reinterpret_cast<elf32_address_t *>(_relocationBase + rel->r_offset);
				elf32_address_t target;

				uint32_t symnum = ELF32_R_SYM(rel->r_info);
				uint32_t type = ELF32_R_TYPE(rel->r_info);

				assert(type == R_386_JMP_SLOT);

				elf_sym_t *lookup = _symTable + symnum;
				const char *name  = _strTable + lookup->st_name;

				Module *container;
				void *symbol = LookupAddress(name, container);
				if(!symbol)
					return Error(KERN_RESOURCES_MISSING, "Couldn't find symbol '%s'!\n", name);

				target = (elf32_address_t)(symbol);
				*address = target;
			}

			return ErrorNone;
		}

		KernReturn<void> Module::RelocateGOT()
		{
			for(elf_rel_t *rel = _rel; rel < _relLimit; rel ++)
			{
				elf32_address_t *address = reinterpret_cast<elf32_address_t *>(_relocationBase + rel->r_offset);
				elf32_address_t target;

				uint32_t symnum = ELF32_R_SYM(rel->r_info);
				uint32_t type = ELF32_R_TYPE(rel->r_info);

				elf_sym_t *lookup = _symTable + symnum;
				const char *name  = _strTable + lookup->st_name;

				switch(type)
				{
					case R_386_NONE:
						break;

					case R_386_32:
					case R_386_GLOB_DAT:
					{
						Module *container;
						void *symbol = LookupAddress(name, container);

						if(!symbol)
							return Error(KERN_RESOURCES_MISSING, "Couldn't find symbol '%s'!\n", name);

						target = reinterpret_cast<elf32_address_t>(symbol);
						*address = target + *address;
						break;
					}

					case R_386_PC32:
					{
						Module *container;
						void *symbol = LookupAddress(name, container);

						if(!symbol)
							return Error(KERN_RESOURCES_MISSING, "Couldn't find symbol '%s'!\n", name);

						target = reinterpret_cast<elf32_address_t>(symbol);
						*address += target - reinterpret_cast<elf32_address_t>(address);
						break;
					}

					case R_386_RELATIVE:
						*address += _relocationBase;
						break;

					default:
						return Error(KERN_INVALID_ARGUMENT, "Unknown relocation type %d", type);
				}
			}

			return ErrorNone;
		}


		elf_sym_t *Module::GetSymbolWithName(const char *name)
		{
			uint32_t hash = elf_hash(name);
			uint32_t symnum = _buckets[hash % _bucketCount];

			while(symnum != 0)
			{
				elf_sym_t *symbol = _symTable + symnum;
				const char *str = _strTable + symbol->st_name;

				if(strcmp(str, name) == 0)
					return symbol;

				symnum = _chains[symnum];
			}

			return nullptr;
		}

		void *Module::GetSymbolAddressWithName(const char *name)
		{
			elf_sym_t *symbol = GetSymbolWithName(name);
			if(symbol && symbol->st_value != 0)
			{
				void *ptr = reinterpret_cast<void *>(_relocationBase + symbol->st_value);
				return ptr;
			}

			return nullptr;
		}

		void *Module::LookupAddress(const char *name, Module *&outLib)
		{
			elf_sym_t *lookup = GetSymbolWithName(name);
			if(!lookup)
				return nullptr;

			if(lookup->st_name == kModuleSymbolStubName)
			{
				outLib = this;
				return reinterpret_cast<void *>(lookup->st_value);
			}
			else if(ELF32_ST_BIND(lookup->st_info) == STB_LOCAL)
			{
				outLib = this;
				return reinterpret_cast<void *>(_relocationBase + lookup->st_value);
			}
			else
			{
				const char *name  = _strTable + lookup->st_name;
				void *address;

				// Search through the dependencies breadth first
				if(_dependencies)
				{
					IO::Array *array = IO::Array::Alloc()->Init();
					array->AddObject(_dependencies);

					for(size_t i = 0; i < array->GetCount(); i ++)
					{
						IO::Array *dependencies = array->GetObjectAtIndex<IO::Array>(i);

						dependencies->Enumerate<Dependency>([&](Dependency *dependency, size_t index, bool &stop) {

							Module *module = dependency->GetDependency();

							address = module->GetSymbolAddressWithName(name);
							if(address)
							{
								outLib = module;
								stop = true;
							}

							if(module->_dependencies)
								array->AddObject(module->_dependencies);

						});

						if(address != 0)
						{
							array->Release();
							return address;
						}
					}
				}

				// Return the current module
				if(lookup->st_name == kModuleSymbolStubName)
				{
					outLib = this;
					return reinterpret_cast<void *>(lookup->st_value);
				}
				else
				{
					outLib = this;
					return reinterpret_cast<void *>(_relocationBase + lookup->st_value);
				}
			}
		}

		bool Module::Start()
		{
			IOAssert(_type == Type::Extension, "Only extensions can be started");

			if(_initArrayCount > 0)
			{
				void (*initFunction)();

				for(size_t i = 0; i < _initArrayCount; i ++)
				{
					if(_initArray[i] == 0 || _initArray[i] == UINT32_MAX)
						continue;

					initFunction = reinterpret_cast<void (*)()>(_initArray[i]);
					initFunction();
				}
			}

			return _start(this);
		}

		void Module::Stop()
		{
			IOAssert(_type == Type::Extension, "Only extensions can be stopped");
			_stop(this);
		}
	}
}
