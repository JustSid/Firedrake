//
//  stubs.h
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

#include "libkern.h"
#include "libc/stddef.h"
#include "libc/stdbool.h"

void kprintf(__unused const char *format, ...)
{}
void kputs(__unused const char *string)
{}
void knputs(__unused const char *string, __unused unsigned int length)
{}

void panic(__unused const char *format, ...)
{
	while(1) {}
}


void *kalloc(__unused size_t size)
{
	return NULL;
}
void kfree(__unused void *ptr)
{}


void *__libio_getIOCatalogue()
{
	return NULL;
}
void *__libio_getIONull()
{
	return NULL;
}
void *__libio_getIORootRegistry()
{
	return NULL;
}
void __libio_publishDisplay(__unused void *display)
{}


void thread_create(__unused void (*entry)(void *), __unused void *argument)
{}

void thread_yield()
{}


void register_interrupt(__unused uint8_t vector, __unused void *argument, __unused InterruptHandler handler)
{}


void __libkern_dispatchKeyboardEvent(__unused uint32_t keyCode, __unused bool keyDown)
{}

int atexit(void (*function)(void))
{
	return 0;
}

void __cxa_atexit()
{}

void *__libkern_dma_map(__unused uintptr_t physical, __unused size_t pages)
{
	return NULL;
}
void __libkern_dma_free(__unused void *virt, __unused size_t pages)
{}
