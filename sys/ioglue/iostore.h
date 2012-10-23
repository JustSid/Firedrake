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
#include <container/atree.h>
#include <memory/vmemory.h>

#include "iolibrary.h"
#include "iomodule.h"

elf_sym_t *io_storeLookupSymbol(io_library_t *library, uint32_t symbol, io_library_t **outLib);

io_library_t *io_storeLibraryWithAddress(vm_address_t address);
io_library_t *io_storeLibraryWithName(const char *name);

bool io_storeAddLibrary(io_library_t *library);
void io_storeRemoveLibrary(io_library_t *library);

bool io_init(void *unused);

#endif /* _IOSTORE_H_ */
