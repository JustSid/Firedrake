//
//  syscall.h
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

#ifndef _KERN_SYSCALL_H_
#define _KERN_SYSCALL_H_

#include <prefix.h>
#include <libc/sys/syscall.h>
#include <kern/kern_return.h>
#include <machine/memory/memory.h>

namespace OS
{
	typedef KernReturn<uint32_t> (*SyscallHandler)(uint32_t &esp, void *arguments);

	class SyscallScopedMapping
	{
	public:
		SyscallScopedMapping(const void *pointer, size_t size);
		~SyscallScopedMapping();

		template<class T>
		KernReturn<T *> GetMemory() const
		{
			KernReturn<void *> result = __GetMemory();
			if(!result.IsValid())
				return result.GetError();

			return reinterpret_cast<T *>(result.Get());
		}

	private:
		KernReturn<void *> __GetMemory() const
		{
			if(!_pointer)
				return Error(KERN_NO_MEMORY);

			return reinterpret_cast<void *>(_pointer);
		}

		vm_address_t _address;
		vm_address_t _pointer;
		size_t _pages;
	};

	KernReturn<void> SetSyscallHandler(uint32_t number, SyscallHandler handler, size_t argSize, bool isUnixStyle);
	KernReturn<void> SyscallInit();
}

#endif /* _KERN_SYSCALL_H_ */
