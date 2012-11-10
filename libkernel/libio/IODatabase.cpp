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
#include "IOService.h"
#include "IOAutoreleasePool.h"
#include "IORuntime.h"
#include "IOLog.h"

static IODatabase *_database = 0;

IOString *IOServiceAttributeIdentifier = 0;
IOString *IOServiceAttributeFamily = 0;
IOString *IOServiceAttributeProperties = 0;

bool IODatabase::init()
{
	_lock = IO_SPINLOCK_INIT;
	_symbols = new __IOPointerDictionary;
	_symbols->retainCount = 1;
	_symbols->initWithCapacity(20);

	_services = 0;
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

			IOServiceAttributeIdentifier = IOString::alloc()->initWithCString("IOServiceAttributeIdentifier");
			IOServiceAttributeFamily     = IOString::alloc()->initWithCString("IOServiceAttributeFamily");
			IOServiceAttributeProperties = IOString::alloc()->initWithCString("IOServiceAttributeProperties");
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

	IOSpinlockUnlock(&_lock);

	return (exists == false);
}

void IODatabase::unpublishSymbol(IOSymbol *UNUSED(symbol))
{
}


class IODatabaseEntry : public IOObject
{
public:
	virtual IODatabaseEntry *init()
	{
		if(IOObject::init())
		{
			_attributes = 0;
			_symbol = 0;
		}

		return this;
	}

	virtual void free()
	{
		_attributes->release();
	}

	static IODatabaseEntry *withSymbolAndAttributes(IOSymbol *symbol, IODictionary *attributes)
	{
		IODatabaseEntry *entry = IODatabaseEntry::alloc()->init();
		if(!entry)
			return entry;

		entry->_symbol = symbol;
		entry->_attributes = (IODictionary *)attributes->retain();

		return entry->autorelease();
	}

	IOSymbol *getSymbol()
	{
		return _symbol;
	}

	IODictionary *getAttributes()
	{
		return _attributes;
	}

	IODeclareClass(IODatabaseEntry)

private:
	IOSymbol *_symbol;
	IODictionary *_attributes;
};

IORegisterClass(IODatabaseEntry, IOObject);

uint32_t IODatabase::scoreForEntryMatch(IODatabaseEntry *entry, IODictionary *attributes)
{
	IODictionary *entryAttributes = entry->getAttributes();

	IOString *identifier = (IOString *)attributes->objectForKey(IOServiceAttributeIdentifier);
	IOString *entryIdentifer = (IOString *)attributes->objectForKey(IOServiceAttributeIdentifier);

	if(!identifier || identifier->isEqual(entryIdentifer))
	{
		IOString *family = (IOString *)attributes->objectForKey(IOServiceAttributeFamily);
		if(family)
		{
			IOString *entryFamily = (IOString *)entryAttributes->objectForKey(IOServiceAttributeFamily);

			if(!family->isEqual(entryFamily))
				return 0;
		}

		IODictionary *properties = (IODictionary *)attributes->objectForKey(IOServiceAttributeProperties);
		IODictionary *entryProperties = (IODictionary *)entryAttributes->objectForKey(IOServiceAttributeProperties);

		if(!properties)
			return 1;

		if(!entryProperties)
			return 0;

		IOIterator *iterator = properties->keyIterator();
		IOObject *key;

		while((key = iterator->nextObject()))
		{
			IOObject *object = properties->objectForKey(key);
			IOObject *entryObject = entryProperties->objectForKey(key);

			if(!object->isEqual(entryObject))
				return 0;
		}

		return entryProperties->count() - properties->count();
	}

	return 0;
}

IOService *IODatabase::serviceMatchingAttributes(IODictionary *attributes)
{
	if(!_services)
		return 0;

	IOSymbol *bestMatch = 0;
	uint32_t bestScore  = 0;

	IOAutoreleasePool *pool = IOAutoreleasePool::alloc();
	pool->init();

	IOSpinlockLock(&_lock);

	IOIterator *iterator = _services->objectIterator();
	IODatabaseEntry *entry;

	while((entry = (IODatabaseEntry *)iterator->nextObject()))
	{
		uint32_t score = scoreForEntryMatch(entry, attributes);
		if(score > bestScore)
		{
			bestMatch = entry->symbol();
			bestScore = score;
		}
	}

	IOSpinlockUnlock(&_lock);
	pool->release();

	if(bestMatch)
	{
		IOService *service = (IOService *)bestMatch->alloc();
		service->init();

		return (IOService *)service->autorelease();
	}

	return 0;
}

IOReturn IODatabase::registerService(IOSymbol *symbol, IODictionary *attributes)
{
	IOSpinlockLock(&_lock);

	if(!_services)
	{
		_services = IODictionary::alloc();
		_services->init();
	}

	IOString *identifier = (IOString *)attributes->objectForKey(IOServiceAttributeIdentifier);
	IODatabaseEntry *entry = (IODatabaseEntry *)_services->objectForKey(identifier);

	if(entry)
	{
		IOLog("IODatabase::registerService(), identifier %@ already taken!", identifier);
		IOSpinlockUnlock(&_lock);

		return kIOReturnSlotTaken;
	}

	entry = IODatabaseEntry::withSymbolAndAttributes(symbol, attributes);
	_services->setObjectForKey(entry, identifier);
	
	IOSpinlockUnlock(&_lock);
	return kIOReturnSuccess;
}

IOReturn IODatabase::unregisterService(IOSymbol *UNUSED(symbol))
{
	return kIOReturnSuccess;
}
