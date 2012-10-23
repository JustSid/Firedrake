//
//  IODatabase.cpp
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

#include <libc/string.h>
#include "IODatabase.h"
#include "IORuntime.h"
#include "IOLog.h"

static IODatabase *_database = 0;

bool IODatabase::init()
{
	_lock = IO_SPINLOCK_INIT;
	_symbols = new __IOPointerDictionary;
	_symbols->retainCount = 1;
	_symbols->initWithCapacity(20);

	_symbolsFixed = false;
	_dictionaryFixed = false;

	_database = this;
	return (_symbols != 0);
}

IODatabase *IODatabase::database()
{
	if(!_database)
	{
		_database = new IODatabase;
		bool result = _database->init();

		if(!result)
			panic("Couldn't init IODatabase!");
	}
	
	return _database;
}


IOSymbol *IODatabase::symbolWithName(IOString *name)
{
	IOSpinlockLock(&_lock);
	IOSymbol *symbol = (IOSymbol *)_symbols->pointerForKey(name);
	IOSpinlockUnlock(&_lock);

	return symbol;
}

IOSymbol *IODatabase::symbolWithName(const char *name)
{
	IOString *string = IOString::withCString(name);
	IOSymbol *symbol = symbolWithName(string);
	string->release();

	return symbol;
}


void IODatabase::fixSymbols(IOSymbol *stringSymbol)
{
	// Tell all strings in the database that they are strings...
	IODictionaryBucket **buckets = _symbols->_buckets;
	for(size_t i=0; i<_symbols->_capacity; i++)
	{
		IODictionaryBucket *bucket = buckets[i];
		while(bucket)
		{
			if(bucket->key)
			{
				IOString *key = (IOString *)bucket->key;
				key->_symbol = stringSymbol;
			}

			bucket = bucket->next;
		}
	}
}

bool IODatabase::publishSymbol(IOSymbol *symbol)
{
	IOSpinlockLock(&_lock);

	IOString *key = (IOString *)symbol->name();
	bool exists = (_symbols->pointerForKey(key) != 0);

	if(!exists)
		_symbols->setPointerForKey(symbol, key);


	if(!_symbolsFixed)
	{
		if(strcmp(symbol->name()->CString(), "IOString") == 0)
		{
			fixSymbols(symbol);
			_symbolsFixed = true;
		}
	}

	if(!_dictionaryFixed)
	{
		if(strcmp(symbol->name()->CString(), "__IOPointerDictionary"))
		{	
			_symbols->_symbol = symbol;
			_dictionaryFixed = true;
		}
	}

	//IOLog("Published %s", symbol->name()->CString());
	IOSpinlockUnlock(&_lock);

	return (exists == false);
}

void IODatabase::unpublishSymbol(IOSymbol *UNUSED(symbol))
{
}
