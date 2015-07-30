//
//  IOFunction.h
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

#ifndef _IOFUNCTION_H_
#define _IOFUNCTION_H_

#include <libcpp/type_traits.h>
#include "IORuntime.h"

namespace IO
{
	template<class T>
	class Function
	{};

	template<class Res, class... Args>
	class Function<Res (Args...)>
	{
	public:
		Function() :
			_callback(nullptr)
		{}
		template<typename F>
		Function(F &&f) :
			_callback(new ImplementationType<F>(std::move(f)))
		{}
		
		Function(Function &&other) :
			_callback(std::move(other._callback))
		{
			other._callback = nullptr;
		}

		Function &operator =(Function &&other)
		{
			_callback = std::move(other._callback);
			other._callback = nullptr;
		}

		~Function()
		{
			delete _callback;
		}

		Function(const Function&) = delete;
		Function &operator= (const Function&) = delete;
		

		Res operator ()(Args... args) const { return _callback->Call(args...); }
		
	private:
		struct Base
		{
			virtual Res Call(Args... args) const = 0;
			virtual ~Base() {}
		};
		
		template<typename F>
		struct ImplementationType : Base
		{
			ImplementationType(F &&f) :
				_function(std::move(f))
			{}
			
			Res Call(Args... args) const final { return _function(args...); }
			F _function;
		};
		
		Base *_callback;
	};
}

#endif /* _IOFUNCTION_H_ */
