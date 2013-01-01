//
//  IONumber.h
//  libio
//
//  Created by Sidney Just
//  Copyright (c) 2013 by Sidney Just
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

#include "IOTypes.h"
#include "IOObject.h"

class IONumber : public IOObject
{
public:
	IONumber *initWithInt8(int8_t val);
	IONumber *initWithInt16(int16_t val);
	IONumber *initWithInt32(int32_t val);
	IONumber *initWithInt64(int64_t val);

	IONumber *initWithUInt8(uint8_t val);
	IONumber *initWithUInt16(uint16_t val);
	IONumber *initWithUInt32(uint32_t val);
	IONumber *initWithUInt64(uint64_t val);

	static IONumber *withInt8(int8_t val);
	static IONumber *withInt16(int16_t val);
	static IONumber *withInt32(int32_t val);
	static IONumber *withInt64(int64_t val);

	static IONumber *withUInt8(uint8_t val);
	static IONumber *withUInt16(uint16_t val);
	static IONumber *withUInt32(uint32_t val);
	static IONumber *withUInt64(uint64_t val);

	int8_t int8Value() const;
	int16_t int16Value() const;
	int32_t int32Value() const;
	int64_t int64Value() const;

	uint8_t uint8Value() const;
	uint16_t uint16Value() const;
	uint32_t uint32Value() const;
	uint64_t uint64Value() const;

	virtual hash_t hash() const;
	virtual bool isEqual(const IOObject *other) const;

	virtual IOString *description() const;

private:
	typedef enum
	{
		kIONumberTypeInt8,
		kIONumberTypeInt16,
		kIONumberTypeInt32,
		kIONumberTypeInt64,
		kIONumberTypeUInt8,
		kIONumberTypeUInt16,
		kIONumberTypeUInt32,
		kIONumberTypeUInt64,
	} Type;

	IONumber *initWithBuffer(Type type, void *blob, size_t size);
	virtual void free();

	Type _type;
	void *_buffer;

	IODeclareClass(IONumber)
};

#endif /* _IONUMBER_H_ */
