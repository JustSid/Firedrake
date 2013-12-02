//
//  cme.h
//  Firedrake
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

#ifndef _CME_H_
#define _CME_H_

#include <libc/stdint.h>
#include <libc/stddef.h>
#include <kern/kern_return.h>

static inline uint32_t cme_resolve_call(uint8_t *buffer, uintptr_t function)
{
	uint32_t base = reinterpret_cast<uint32_t>(buffer);
	return function - base;
}

static inline uint8_t *cme_find_instruction(uint8_t *buffer, size_t limit, uint8_t instruction)
{
	size_t index = 0;
	while(*buffer != instruction)
	{
		buffer ++;
		index  ++;

		if(index >= limit)
			return nullptr;
	}

	return buffer;
}


static inline kern_return_t cme_fix_call_simple(uint8_t *buffer, size_t limit, void *func)
{
	buffer = cme_find_instruction(buffer, limit, 0xe8);
	if(!buffer)
		return KERN_FAILURE;

	uint32_t *call = reinterpret_cast<uint32_t *>(buffer + 1);
	*call = cme_resolve_call(reinterpret_cast<uint8_t *>(call + 1), reinterpret_cast<uintptr_t>(func));

	return KERN_SUCCESS;
}

static inline kern_return_t cme_fix_ljump16(uint8_t *buffer, size_t limit, void *target)
{
	buffer = cme_find_instruction(buffer, limit, 0xea);
	if(!buffer)
		return KERN_FAILURE;

	uint16_t *jump = reinterpret_cast<uint16_t *>(buffer + 1);
	*jump ++ = static_cast<uint16_t>(reinterpret_cast<uint32_t>(target));

	return KERN_SUCCESS;
}

static inline kern_return_t cme_fix_inst16(uint8_t *buffer, size_t limit, uint8_t instruction, uint16_t value)
{
	buffer = cme_find_instruction(buffer, limit, instruction);
	if(!buffer)
		return KERN_FAILURE;

	uint16_t *inst = reinterpret_cast<uint16_t *>(buffer + 1);
	*inst = value;

	return KERN_SUCCESS;
}

#endif /* _CME_H_ */
