//
//  IODatabase.cpp
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

#include <libkernel/string.h>
#include <libkernel/base.h>

#include "IODatabase.h"
#include "IOService.h"
#include "IOAutoreleasePool.h"
#include "IORuntime.h"
#include "IOLog.h"

static IODatabase *_database = 0;

IOString *IOServiceAttributeFamily = 0;
IOString *IOServiceAttributeProperties = 0;

bool IODatabase::init()
{
	_lock = KERN_SPINLOCK_INIT;
	_readLock = KERN_SPINLOCK_INIT;

	_symbols = new __IOPointerDictionary;
	_symbols->_retainCount = 1;
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
	bool acquiredLock = kern_spinlock_tryLock(&_readLock);

	IOSymbol *symbol = (IOSymbol *)_symbols->pointerForKey(name);
	
	if(acquiredLock)
		kern_spinlock_unlock(&_readLock);

	return symbol;
}

IOSymbol *IODatabase::symbolWithName(const char *name)
{
	IOString *string = IOString::alloc()->initWithCString(name);
	IOSymbol *symbol = symbolWithName(string);
	string->release();

	return symbol;
}


void IODatabase::fixSymbols(IOSymbol *symbol)
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
				key->_symbol = symbol;
			}

			bucket = bucket->next;
		}
	}
}

bool IODatabase::publishSymbol(IOSymbol *symbol)
{
	kern_spinlock_lock(&_readLock);
	kern_spinlock_lock(&_lock);

	IOString *key = (IOString *)symbol->name();
	bool exists = (_symbols->pointerForKey(key) != 0);
	
	if(!exists)
		_symbols->setPointerForKey(symbol, key);

	if(!_symbolsFixed)
	{
		if(strcmp(symbol->name()->CString(), "IOString") == 0)
		{
			fixSymbols(symbol);

			_stringSymbol = symbol;
			_symbolsFixed = true;

			IOServiceAttributeFamily     = IOString::alloc()->initWithCString("IOServiceAttributeFamily");
			IOServiceAttributeProperties = IOString::alloc()->initWithCString("IOServiceAttributeProperties");
		}
	}
	else
	{
		key->_symbol = _stringSymbol;
	}

	if(!_dictionaryFixed)
	{
		if(strcmp(symbol->name()->CString(), "__IOPointerDictionary"))
		{	
			_symbols->_symbol = symbol;
			_dictionaryFixed = true;
		}
	}

	kern_spinlock_unlock(&_lock);
	kern_spinlock_unlock(&_readLock);

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

	uint32_t scoreForAttributes(IODictionary *attributes)
	{
		uint32_t score = 1;

		IODictionary *properties = (IODictionary *)attributes->objectForKey(IOServiceAttributeProperties);
		IODictionary *_properties = (IODictionary *)_attributes->objectForKey(IOServiceAttributeProperties);

		if(!properties || !_properties)
			return score;

		IOIterator *iterator = _properties->keyIterator();
		IOObject *key;

		while((key = iterator->nextObject()))
		{
			IOObject *object = properties->objectForKey(key);
			IOObject *_object = _properties->objectForKey(key);

			if(object && !object->isEqual(_object))
				return 0;

			score ++;
		}

		return score;
	}

	IODeclareClass(IODatabaseEntry)

private:
	IOSymbol *_symbol;
	IODictionary *_attributes;
};

IORegisterClass(IODatabaseEntry, IOObject);

IOService *IODatabase::serviceMatchingAttributes(IODictionary *attributes)
{
	if(!_services)
		return 0;

	kern_spinlock_lock(&_readLock);

	IOObject *family = attributes->objectForKey(IOServiceAttributeFamily);
	IOArray *services = (IOArray *)_services->objectForKey(family);

	if(services)
	{
		IOSymbol *bestMatch = 0;
		uint32_t bestScore  = 0;

		for(uint32_t i=0; i<services->count(); i++)
		{
			IODatabaseEntry *entry = (IODatabaseEntry *)services->objectAtIndex(i);

			uint32_t score = entry->scoreForAttributes(attributes);
			if(score > bestScore)
			{
				bestMatch = entry->getSymbol();
				bestScore = score;
			}
		}

		if(bestMatch)
		{
			IOService *service = (IOService *)bestMatch->alloc();
			service->init();

			kern_spinlock_unlock(&_readLock);
			
			return service->autorelease();
		}
	}


	kern_spinlock_unlock(&_readLock);
	return 0;
}

IOReturn IODatabase::registerService(IOSymbol *symbol, IODictionary *attributes)
{
	kern_spinlock_lock(&_readLock);
	kern_spinlock_lock(&_lock);

	if(!_services)
	{
		_services = IODictionary::alloc();
		_services->init();
	}

	IOString *family = (IOString *)attributes->objectForKey(IOServiceAttributeFamily);
	IOArray *familyServices = (IOArray *)_services->objectForKey(family);

	if(!familyServices)
	{
		familyServices = IOArray::alloc()->init();
		_services->setObjectForKey(familyServices, family);
		familyServices->release();
	}

	IODatabaseEntry *entry = IODatabaseEntry::withSymbolAndAttributes(symbol, attributes);
	familyServices->addObject(entry);
	
	kern_spinlock_unlock(&_lock);
	kern_spinlock_unlock(&_readLock);

	return kIOReturnSuccess;
}

IOReturn IODatabase::unregisterService(IOSymbol *UNUSED(symbol))
{
	return kIOReturnSuccess;
}
