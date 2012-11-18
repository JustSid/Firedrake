//
//  IODatabase.h
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

#ifndef _IODATABASE_H_
#define _IODATABASE_H_

#include <libkernel/spinlock.h>

#include "IOObject.h"
#include "IODictionary.h"
#include "IOString.h"
#include "IOSymbol.h"

class IOService;
class IODatabaseEntry;

class IODatabase
{
friend class IOSymbol;
friend class IOService;
public:
	static IODatabase *database();

	IOSymbol *symbolWithName(IOString *name);
	IOSymbol *symbolWithName(const char *name);

private:
	bool init();

	void fixSymbols(IOSymbol *symbol);

	bool publishSymbol(IOSymbol *symbol);
	void unpublishSymbol(IOSymbol *symbol);

	IOReturn registerService(IOSymbol *symbol, IODictionary *attributes);
	IOReturn unregisterService(IOSymbol *symbol);

	uint32_t scoreForEntryMatch(IODatabaseEntry *entry, IODictionary *attributes);
	IOService *serviceMatchingAttributes(IODictionary *attributes);

	bool _symbolsFixed;
	bool _dictionaryFixed;

	kern_spinlock_t _lock;

	__IOPointerDictionary *_symbols;
	IODictionary *_services;
};

#endif /* _IODATABASE_H_ */
