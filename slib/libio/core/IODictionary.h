//
//  IODictionary.h
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

#ifndef _IODICTIONARY_H_
#define _IODICTIONARY_H_

#include "IOObject.h"
#include "IORuntime.h"
#include "IOFunction.h"

namespace IO
{
	class Dictionary : public Object
	{
	public:
		Dictionary *Init();
		Dictionary *InitWithCapacity(size_t capacity);

		template<class T = Object>
		T *GetObjectForKey(Object *key) const
		{
			Object *result = __GetPrimitiveObjectForKey(key);
			if(result)
				return result->Downcast<T>();

			return nullptr;
		}

		void SetObjectForKey(Object *object, Object *key);
		void RemoveObjectForKey(Object *key);
		void RemoveAllObjects();

		size_t GetCount() const;

		template<class T, class K>
		void Enumerate(const Function<void (T *, K *, bool &)> &callback) const
		{
			__Enumerate([&](Object *object, Object *key, bool &stop) {
				callback(static_cast<T *>(object), static_cast<K *>(key), stop);
			});
		}
		
	protected:
		Dictionary();
		void Dealloc() override;

	private:
		class Internal;

		void __Enumerate(const Function<void (Object *, Object *, bool &)> &callback) const;
		Object *__GetPrimitiveObjectForKey(Object *key) const;

		PIMPL<Internal> _internals;

		IODeclareMeta(Dictionary)
	};
}

#endif /* _IODICTIONARY_H_ */
