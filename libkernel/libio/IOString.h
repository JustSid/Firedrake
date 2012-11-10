//
//  IOString.h
//  libio
//
//  Created by Sidney Just
//  Copyright (c) 2012 by Sidney Just
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

#include "IOObject.h"

class IOString : public IOObject
{
public:
	IOString *initWithString(const IOString *other);
	IOString *initWithCString(const char *cstring);
	IOString *initWithFormat(const char *cstring, ...);
	IOString *initWithVariadic(const char *format, va_list args);

	static IOString *withString(const IOString *other);
	static IOString *withCString(const char *cstring);
	static IOString *withFormat(const char *format, ...);

	virtual IOString *description() const;

	virtual hash_t hash() const;
	virtual bool isEqual(const IOObject *object) const;

	size_t length() const;
	const char *CString() const;

protected:
	virtual IOString *init(); // Designated initializer!
	virtual void free();

private:
	char *_string;
	size_t _length;

	IODeclareClass(IOString)
};

#endif /* _IOSTRING_H_ */
