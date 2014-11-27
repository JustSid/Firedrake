//
//  IONumber.h
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

#ifndef _IONUMBER_H_
#define _IONUMBER_H_

#include "IOObject.h"

namespace IO
{
	class Number : public Object
	{
	public:
		enum class Type
		{
			Int8,
			Int16,
			Int32,
			Int64,
			Uint8,
			Uint16,
			Uint32,
			Uint64,
			Boolean
		};
		
		Number *InitWithBool(bool value);
		
		Number *InitWithInt8(int8_t value);
		Number *InitWithInt16(int16_t value);
		Number *InitWithInt32(int32_t value);
		Number *InitWithInt64(int64_t value);
		
		Number *InitWithUint8(uint8_t value);
		Number *InitWithUint16(uint16_t value);
		Number *InitWithUint32(uint32_t value);
		Number *InitWithUint64(uint64_t value);
		
		bool GetBoolValue() const;
		
		int8_t GetInt8Value() const;
		int16_t GetInt16Value() const;
		int32_t GetInt32Value() const;
		int64_t GetInt64Value() const;
		
		uint8_t GetUint8Value() const;
		uint16_t GetUint16Value() const;
		uint32_t GetUint32Value() const;
		uint64_t GetUint64Value() const;
		
		size_t GetHash() const override;
		bool IsEqual(Object *other) const override;
		
		Type GetType() const { return _type; }

	protected:
		void Dealloc() override;
		
	private:
		static size_t SizeForType(Type type);
		void CopyData(const void *data, size_t size, Type type);
		
		uint8_t *_buffer;
		Type _type;
		
		IODeclareMeta(Number)
	};
}

#endif /* _IONUMBER_H_ */