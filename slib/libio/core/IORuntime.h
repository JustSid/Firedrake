//
//  IORuntime.h
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

#ifndef _IORUNTIME_H_
#define _IORUNTIME_H_

#include <libc/stdint.h>
#include <libcpp/type_traits.h>

#if __KERNEL
#include <kern/panic.h>
#else
#include <libkern.h>
#endif

namespace IO
{
	constexpr size_t kNotFound = static_cast<size_t>(-1);

	template <class T>
	class PIMPL
	{
	public:
		template<class ...Args>
		PIMPL(Args &&...args) :
			_ptr(new T(std::forward<Args>(args)...))
		{}

		~PIMPL()
		{
			delete _ptr;
		}
		
		operator T* ()
		{
			return _ptr;
		}
		operator const T* () const
		{
			return _ptr;
		}
		
		T *operator ->()
		{
			return _ptr;
		}
		const T *operator ->() const
		{
			return _ptr;
		}
		
	private:
		T *_ptr;
	};

	class Object;
	typedef void (*IOFunctionPtr)();

	static inline IOFunctionPtr __ObjectMemberFunctionToPointer(const Object *object, void (Object::*func)(void))
	{
		union {
			void (Object::*fIn)(void);
			uintptr_t fVTOffset;
			IOFunctionPtr fPFN;
		} map;

		map.fIn = func;

		// Virtual function
		if(map.fVTOffset & 0x1)
		{
			union {
				const Object *fObj;
				IOFunctionPtr **vtablep;
			} u;
			u.fObj = object;

			return *(IOFunctionPtr *)(((uintptr_t)*u.vtablep) + map.fVTOffset - 1);
		}

		return map.fPFN;
	}
}

#define IOMemberFunctionCast(type, object, func) \
	reinterpret_cast<type>(__ObjectMemberFunctionToPointer(object, (void (IO::Object::*)(void))func))

#define IOAssert(e, message) __builtin_expect(!(e), 0) ? panic("%s:%i: Assertion \'%s\' failed, %s", __func__, __LINE__, #e, message) : (void)0

#endif /* _IORUNTIME_H_ */
