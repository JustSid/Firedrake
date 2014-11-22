//
//  loader.h
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

#ifndef _LOADER_H_
#define _LOADER_H_

#include <prefix.h>
#include <kern/kern_return.h>
#include <machine/memory/memory.h>
#include <libc/stddef.h>
#include <libc/stdint.h>

namespace Sys
{
	class Executable
	{
	public:
		Executable(VM::Directory *directory);
		~Executable();

		VM::Directory *GetDirectory() const { return _directory; }
		vm_address_t GetEntry() const { return _entry; }

		uint32_t GetState() const { return _state; }

	private:
		KernReturn<void> Initialize(void *data);

		uint32_t _state;

		VM::Directory *_directory;
		vm_address_t _entry;

		uintptr_t _physical;
		vm_address_t _virtual;
		size_t _pages;
	};
}

#endif /* _LOADER_H_ */
