//
//  IOArray.h
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

#ifndef _IOARRAY_H_
#define _IOARRAY_H_

#include "IOObject.h"
#include "IORuntime.h"
#include "IOFunction.h"

namespace IO
{
	class Array : public Object
	{
	public:
		Array *Init();
		Array *InitWithCapacity(size_t size);
		Array *InitWithArray(const Array *other);

		void AddObject(Object *object);
		void AddObjectsFromArray(const Array *other);

		void RemoveObject(Object *object);
		void RemoveObjectAtIndex(size_t index);
		void RemoveAllObjects();

		size_t GetIndexOfObject(Object *object) const;
		size_t GetCount() const;

		template<class T>
		T *GetObjectAtIndex(size_t index) const
		{
			return static_cast<T *>(__GetObjectAtIndex(index));
		}

		template<class T>
		T *GetFirstObject() const
		{
			return static_cast<T *>(__GetFirstObject());
		}

		template<class T>
		T *GetLastObject() const
		{
			return static_cast<T *>(__GetLastObject());
		}

		template<class T>
		void Enumerate(const Function<void (T *, size_t, bool &)> &callback) const
		{
			bool stop = false;

			for(size_t i = 0; i < _count; i ++)
			{
				callback(static_cast<T *>(_data[i]), i, stop);
				if(stop)
					return;
			}
		}

	protected:
		void Dealloc() override;

	private:
		void AssertCapacity(size_t required);

		Object *__GetObjectAtIndex(size_t index) const;
		Object *__GetFirstObject() const;
		Object *__GetLastObject() const;

		size_t _count;
		size_t _capacity;
		Object **_data;

		IODeclareMeta(Array)
	};
}

#endif /* _IOARRAY_H_ */
