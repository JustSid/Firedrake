//
//  iostore.h
//  Firedrake
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

#ifndef _IOSTORE_H_
#define _IOSTORE_H_

#include <types.h>
#include <system/lock.h>
#include <container/list.h>
#include <memory/vmemory.h>
#include "iolink.h"

typedef struct
{
	spinlock_t lock;
	list_t *libraries;
} io_store_t;

elf_sym_t *io_storeFindSymbol(io_library_t *library, uint32_t symbol, io_library_t **outLib);
//elf_sym_t *io_storeFindSymbolPLT(io_library_t *library, uint32_t symbol, io_library_t **outLib);

void *io_storeLookupSymbol(const char *name);
void *io_storeLookupSymbolInLibrary(io_library_t *library, const char *name);

io_library_t *io_storeLibraryWithAddress(vm_address_t address);
io_library_t *io_storeLibraryWithName(const char *name);
io_store_t *io_storeGetStore();

bool io_init(void *unused);

#endif /* _IOSTORE_H_ */
