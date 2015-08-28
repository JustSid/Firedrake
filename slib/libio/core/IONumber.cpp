//
//  IONumber.cpp
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

#include "IORuntime.h"
#include "IONumber.h"

namespace IO
{
	__IODefineIOCoreMeta(Number, Object)
	
#define NumberPrimitiveAccess(type, target) static_cast<target>(*((type *)_buffer))
#define NumberIsSignedInteger(type) (type == Type::Int8 || type == Type::Int16 || type == Type::Int32 || type == Type::Int64)
#define NumberIsUnsignedInteger(type) (type == Type::Uint8 || type == Type::Uint16 || type == Type::Uint32 || type == Type::Uint64)
#define NumberIsBoolean(type) (type == Type::Boolean)

#define NumberAccessAndConvert(var, type) \
	do { \
		switch(_type) \
		{ \
			case Type::Boolean: \
				var = NumberPrimitiveAccess(bool, type); \
				break; \
			case Type::Int8: \
				var = NumberPrimitiveAccess(int8_t, type); \
				break; \
			case Type::Uint8: \
				var = NumberPrimitiveAccess(uint8_t, type); \
				break; \
			case Type::Int16: \
				var = NumberPrimitiveAccess(int16_t, type); \
				break; \
			case Type::Uint16: \
				var = NumberPrimitiveAccess(uint16_t, type); \
				break; \
			case Type::Int32: \
				var = NumberPrimitiveAccess(int32_t, type); \
				break; \
			case Type::Uint32: \
				var = NumberPrimitiveAccess(uint32_t, type); \
				break; \
			case Type::Int64: \
				var = NumberPrimitiveAccess(int64_t, type); \
				break; \
			case Type::Uint64: \
				var = NumberPrimitiveAccess(uint64_t, type); \
				break; \
			} \
		} while(0)

	Number *Number::InitWithBool(bool value)
	{
		if(!Object::Init())
			return nullptr;

		CopyData(&value, sizeof(bool), Type::Boolean);
		return this;
	}

	Number *Number::InitWithInt8(int8_t value)
	{
		if(!Object::Init())
			return nullptr;

		CopyData(&value, sizeof(int8_t), Type::Int8);
		return this;
	}
	Number *Number::InitWithInt16(int16_t value)
	{
		if(!Object::Init())
			return nullptr;

		CopyData(&value, sizeof(int16_t), Type::Int16);
		return this;
	}
	Number *Number::InitWithInt32(int32_t value)
	{
		if(!Object::Init())
			return nullptr;

		CopyData(&value, sizeof(int32_t), Type::Int32);
		return this;
	}
	Number *Number::InitWithInt64(int64_t value)
	{
		if(!Object::Init())
			return nullptr;

		CopyData(&value, sizeof(int64_t), Type::Int64);
		return this;
	}
	
	Number *Number::InitWithUint8(uint8_t value)
	{
		if(!Object::Init())
			return nullptr;

		CopyData(&value, sizeof(uint8_t), Type::Uint8);
		return this;
	}
	Number *Number::InitWithUint16(uint16_t value)
	{
		if(!Object::Init())
			return nullptr;

		CopyData(&value, sizeof(uint16_t), Type::Uint16);
		return this;
	}
	Number *Number::InitWithUint32(uint32_t value)
	{
		if(!Object::Init())
			return nullptr;

		CopyData(&value, sizeof(uint32_t), Type::Uint32);
		return this;
	}
	Number *Number::InitWithUint64(uint64_t value)
	{
		if(!Object::Init())
			return nullptr;

		CopyData(&value, sizeof(uint64_t), Type::Uint64);
		return this;
	}
	
	void Number::Dealloc()
	{
		delete [] _buffer;
		Object::Dealloc();
	}
	
	void Number::CopyData(const void *data, size_t size, Type type)
	{
		const uint8_t *source = static_cast<const uint8_t *>(data);
		
		_buffer = new uint8_t[size];
		_type = type;
		
		std::copy(source, source + size, _buffer);
	}

	
	
	bool Number::GetBoolValue() const
	{
		bool value;
		NumberAccessAndConvert(value, bool);
		
		return value;
	}
	
	
	int8_t Number::GetInt8Value() const
	{
		int8_t value;
		NumberAccessAndConvert(value, int8_t);
		
		return value;
	}
	int16_t Number::GetInt16Value() const
	{
		int16_t value;
		NumberAccessAndConvert(value, int16_t);
		
		return value;
	}
	int32_t Number::GetInt32Value() const
	{
		int32_t value;
		NumberAccessAndConvert(value, int32_t);
		
		return value;
	}
	int64_t Number::GetInt64Value() const
	{
		int64_t value;
		NumberAccessAndConvert(value, int64_t);
		
		return value;
	}
	
	
	uint8_t Number::GetUint8Value() const
	{
		uint8_t value;
		NumberAccessAndConvert(value, uint8_t);
		
		return value;
	}
	uint16_t Number::GetUint16Value() const
	{
		uint16_t value;
		NumberAccessAndConvert(value, uint16_t);
		
		return value;
	}
	uint32_t Number::GetUint32Value() const
	{
		uint32_t value;
		NumberAccessAndConvert(value, uint32_t);
		
		return value;
	}
	uint64_t Number::GetUint64Value() const
	{
		uint64_t value;
		NumberAccessAndConvert(value, uint64_t);
		
		return value;
	}
	
	size_t Number::SizeForType(Type type)
	{
		switch(type)
		{
			case Type::Int8:
			case Type::Uint8:
				return 1;
				
			case Type::Int16:
			case Type::Uint16:
				return 2;
				
			case Type::Int32:
			case Type::Uint32:
				return 4;
			
			case Type::Int64:
			case Type::Uint64:
				return 8;
				
			case Type::Boolean:
				return sizeof(bool);
		}

		panic("Number::SizeForType() Unknown type!");
	}
	
	size_t Number::GetHash() const
	{
		const uint8_t *bytes = _buffer;
		const uint8_t *end = bytes + SizeForType(_type);
		
		size_t hash = 0;
		
		while(bytes < end)
		{
			hash ^= (hash << 5) + (hash >> 2) + *bytes;
			bytes ++;
		}
		
		return hash;
	}
	bool Number::IsEqual(Object *other) const
	{
		Number *number = other->Downcast<Number>();
		if(!number)
			return false;
		
		bool integer = ((NumberIsSignedInteger(_type) || NumberIsUnsignedInteger(_type)) && (NumberIsSignedInteger(number->_type) || NumberIsUnsignedInteger(number->_type)));
		if(integer)
		{
			if((NumberIsSignedInteger(_type) && NumberIsSignedInteger(number->_type)) || (NumberIsUnsignedInteger(_type) && NumberIsUnsignedInteger(number->_type)))
			{
				return (GetUint64Value() == number->GetUint64Value());
			}

			uint64_t uint = (NumberIsSignedInteger(_type)) ? number->GetUint64Value() : GetUint64Value();
			if(uint > INT64_MAX)
				return false;

			int64_t  sint = (NumberIsSignedInteger(_type)) ? GetInt64Value() : number->GetInt64Value();
			return (sint == static_cast<int64_t>(uint));
		}

		if(NumberIsBoolean(_type) && NumberIsBoolean(number->_type))
		{
			return (GetBoolValue() == number->GetBoolValue());
		}
		
		return false;
	}
	
#undef NumberPrimitiveAccess
#undef NumberAccessAndConvert
}