//
//  kunittest.h
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2012 by Sidney Just
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
//  documentation files (the "Software"), to deal in the Software without reKUriction, including without limitation 
//  the rights to use, copy, modify, merge, publish, diKUribute, sublicense, and/or sell copies of the Software, 
//  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//  The above copyright notice and this permission notice shall be included in all copies or subKUantial portions of the Software.
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#ifndef _KUNITTEST_H_
#define _KUNITTEST_H_

#include <types.h>
#include <libc/setjmp.h>

typedef enum 
{
	kunit_test_state_waiting,
	kunit_test_state_ran,
	kunit_test_state_failed
} kunit_test_state_t;

typedef void (*kunit_test_function_t)();

typedef struct kunit_test_s
{
	char *name;
	char *description;

	bool continueOnError;

	kunit_test_state_t state;
	uint32_t failures;

	jmp_buf *buffer;
	kunit_test_function_t function;

	void *suite; // The test suite the test belongs to
	struct kunit_test_s *next; // The next test in the test suite
} kunit_test_t;


kunit_test_t *kunit_testCreate(char *name, char *description, kunit_test_function_t function);
void kunit_testDestroy(kunit_test_t *test);

void kunit_testRun(kunit_test_t *test);
void kunit_testFail(kunit_test_t *test, char *description, ...);

#endif /* _KUNITTEST_H_ */
