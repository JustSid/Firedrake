//
//  kunittestsuite.h
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

#ifndef _KUNITTESTSUITE_H_
#define _KUNITTESTSUITE_H_

#include "kunittest.h"

typedef struct
{
	char *name;
	char *description;

	uint32_t run, failed;
	bool destroyAfterRun;

	kunit_test_t *firstTest;
	kunit_test_t *lastTest;
	kunit_test_t *currentTest;
} kunit_test_suite_t;

kunit_test_suite_t *kunit_test_suiteCreate(char *name, char *description, bool destroyAfterRun);
kunit_test_suite_t *kunit_test_suiteCurrent(); // Returns the currently running test suite

kunit_test_t *kunit_testCurrent(); // Returns the currently running test

void kunit_test_suiteDestroy(kunit_test_suite_t *suite); // Automatically also deletes all tests

void kunit_test_suiteAddTest(kunit_test_suite_t *suite, kunit_test_t *test);
void kunit_test_suiteRun(kunit_test_suite_t *suite);

#endif /* _KUNITTESTSUITE_H_ */
