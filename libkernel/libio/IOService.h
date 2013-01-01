//
//  IOService.h
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

#ifndef _IOSERVICE_H_
#define _IOSERVICE_H_

#include "IOTypes.h"
#include "IOObject.h"
#include "IOProvider.h"
#include "IOString.h"
#include "IODictionary.h"
#include "IOArray.h"

extern IOString *IOServiceAttributeFamily; // IOString
extern IOString *IOServiceAttributeProperties; // IODictionary

class IOService : public IOProvider
{
friend class IODatabase;
friend class IOProvider;
public:
	typedef void (*InterruptAction)(IOService *service, void *context, uint32_t interrupt);

	virtual IOService *init();

	virtual bool start(IOProvider *provider);
	virtual void stop(IOProvider *provider);

	IOProvider *provider() { return _provider; }

	static IOReturn registerService(IOSymbol *symbol, IODictionary *attributes);
	static IOReturn unregisterService(IOSymbol *symbol);
	static IODictionary *createMatchingDictionary(IOString *family, IODictionary *properties);

protected:
	virtual void free();

	IOReturn registerInterrupt(uint32_t interrupt, InterruptAction handler, void *context = 0);
	IOReturn unregisterInterrupt(uint32_t interrupt);

private:
	IOProvider *_provider;

	IODeclareClass(IOService)
};

#endif /* _IOSERVICE_H_ */
