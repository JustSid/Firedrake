//
//  IOSet.h
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

#ifndef _IOSET_H_
#define _IOSET_H_

#include "IOObject.h"
#include "IORuntime.h"
#include "IOFunction.h"
#include "IOArray.h"

namespace IO
{	
	class Set : public Object
	{
	public:
		Set *Init();
		Set *InitWithCapacity(size_t capacity);
		
		void AddObject(Object *object);
		void RemoveObject(Object *object);
		void RemoveAllObjects();
		bool ContainsObject(Object *object) const;
		
		void Enumerate(const Function<void (Object *object, bool &stop)> &callback) const;
		
		template<class T>
		void Enumerate(const Function<void (T *object, bool &stop)> &callback) const
		{
			Enumerate([&](Object *object, bool &stop) {
				callback(static_cast<T *>(object), stop);
			});
		}
		
		StrongRef<Array> GetAllObjects() const;
		size_t GetCount() const;
		
	protected:
		Set();
		void Dealloc() override;

	private:
		class Internal;

		PIMPL<Internal> _internals;
		
		IODeclareMeta(Set)
	};
}

#endif /* _IOSET_H_ */
