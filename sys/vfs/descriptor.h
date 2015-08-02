//
//  descriptor.h
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

#ifndef _VFS_DESCRIPTOR_H_
#define _VFS_DESCRIPTOR_H_

#include <prefix.h>
#include <libc/sys/spinlock.h>
#include <kern/kern_return.h>
#include <libc/stddef.h>
#include <libc/stdint.h>
#include <libcpp/bitfield.h>
#include <libcpp/vector.h>

#include <libio/core/IOObject.h>
#include <libio/core/IOString.h>
#include <libio/core/IOArray.h>

namespace VFS
{
	class Instance;
	class Descriptor : public IO::Object
	{
	public:
		CPP_BITFIELD(Flags, uint32_t,
			Persistent = (1 << 0)
		);

		const char *GetName() const { return _name->GetCString(); }
		Flags GetFlags() const { return _flags; }

		virtual KernReturn<Instance *> CreateInstance() = 0;
		virtual void DestroyInstance(Instance *instance) = 0;

		// Must be called at some point
		KernReturn<void> Register();
		KernReturn<void> Unregister();

	protected:
		Descriptor *Init(const char *name, Flags flags);
		void Dealloc() override;

		// Must be called while locked
		void AddInstance(Instance *instance);
		bool RemoveInstance(Instance *instance);

		void Lock();
		void Unlock();

	private:
		IO::String *_name;
		spinlock_t _lock;
		Flags _flags;

		bool _registered;

		IO::Array *_instances;

		IODeclareMetaVirtual(Descriptor)
	};
}

#endif /* _VFS_DESCRIPTOR_H_ */
