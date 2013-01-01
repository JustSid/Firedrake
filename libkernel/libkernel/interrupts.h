//
//  interrupts.h
//  libkernel
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

#ifndef _LIBKERNEL_INTERRUPTS_H_
#define _LIBKERNEL_INTERRUPTS_H_

#include "base.h"
#include "stdint.h"
#include "error.h"

typedef void (*kern_interrupt_handler_t)(void *owner, void *context, uint32_t interrupt);

kern_extern IOReturn kern_requestExclusiveInterrupt(void *owner, void *context, uint32_t *outInterrupt, kern_interrupt_handler_t callback);
kern_extern IOReturn kern_registerForInterrupt(uint32_t interrupt, bool exclusive, void *owner, void *context, kern_interrupt_handler_t callback);

#endif /* _LIBKERNEL_INTERRUPTS_H_ */
