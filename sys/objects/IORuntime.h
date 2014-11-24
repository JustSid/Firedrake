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
#include <kern/panic.h>

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
}

#define IOAssert(e, message) __builtin_expect(!(e), 0) ? panic("%s:%i: Assertion \'%s\' failed, %s", __func__, __LINE__, #e, message) : (void)0

#endif /* _IORUNTIME_H_ */
