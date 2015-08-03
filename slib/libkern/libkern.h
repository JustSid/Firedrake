//
//  libkern.h
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2014 by Sidney Just
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

#ifndef _LIBKERN_H_
#define _LIBKERN_H_

#include "libc/sys/cdefs.h"
#include "libc/stdint.h"
#include "kmod.h"

__BEGIN_DECLS

void kprintf(const char *format, ...) __attribute__((format(printf, 1, 2)));
void kputs(const char *string);
void knputs(const char *string, unsigned int length);

void panic(const char *format, ...) __attribute((noreturn));

void *kalloc(size_t size);
void kfree(void *ptr);

void thread_create(void (*entry)(void *), void *argument);
void thread_yield();


typedef void (*InterruptHandler)(void *argument, uint8_t vector);

void register_interrupt(uint8_t vector, void *argument, InterruptHandler handler);

__END_DECLS

#endif
