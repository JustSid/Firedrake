//
//  dyexecutable.h
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

#ifndef _DYEXECTUABLE_H_
#define _DYEXECTUABLE_H_

#include <types.h>
#include <memory/memory.h>

typedef struct dy_exectuable_s
{
	vm_address_t entry;
	vm_page_directory_t pdirectory;
	struct dy_exectuable_s *source;

	uintptr_t    	pimage;
	vm_address_t 	vimage;
	size_t 			imagePages;

	uint32_t useCount;
} dy_exectuable_t;

dy_exectuable_t *dy_exectuableCreate(vm_page_directory_t pdirectory,  uint8_t *elfstart, size_t elfsize);
dy_exectuable_t *dy_executableCreateWithFile(vm_page_directory_t pdirectory, const char *file);
dy_exectuable_t *dy_exectuableCopy(vm_page_directory_t pdirectory, dy_exectuable_t *source);

void dy_executableRelease(dy_exectuable_t *executable);

#endif /* _DYEXECTUABLE_H_ */
