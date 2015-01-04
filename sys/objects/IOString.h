//
//  IOString.h
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

#ifndef _IOSTRING_H_
#define _IOSTRING_H_

#include <libc/stdarg.h>
#include "IOObject.h"

namespace IO
{
	class String : public Object
	{
	public:
		String *Init();
		String *InitWithCString(const char *string);
		String *InitWithFormat(const char *format, ...);
		String *InitWithFormatAndArguments(const char *format, va_list args);

		size_t GetHash() const override;
		bool IsEqual(Object *other) const override;
		bool IsEqual(const char *string) const;

		size_t GetLength() const;
		char GetCharacterAtIndex(size_t index) const;
		const char *GetCString() const;

	protected:
		void Dealloc() override;

	private:
		void CalculateHash();

		char *_storage;
		size_t _length;
		size_t _hash;
		uint32_t _flags;

		IODeclareMeta(String)
	};
}

#endif /* _IOSTRING_H_ */
