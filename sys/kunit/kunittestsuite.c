//
//  kunittestsuite.c
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

#include <libc/string.h>
#include <system/assert.h>
#include <system/syslog.h>
#include <memory/memory.h>

#include "kunittestsuite.h"

static kunit_test_suite_t *kunit_test_suite_currentSuite = NULL;

kunit_test_suite_t *kunit_test_suiteCreate(char *name, char *description, bool destroyAfterRun)
{
	kunit_test_suite_t *suite = halloc(NULL, sizeof(kunit_test_suite_t));
	if(suite)
	{
		size_t nameLength = name ? strlen(name) : 0;
		size_t descLength = description ? strlen(description) : 0;

		suite->name = name ? halloc(NULL, nameLength + 1) : NULL;
		suite->description = description ? halloc(NULL, descLength + 1) : NULL;

		if((suite->name == NULL && name) || (suite->description == NULL && description))
		{
			if(suite->name)
				hfree(NULL, suite->name);

			if(suite->description)
				hfree(NULL, suite->description);

			hfree(NULL, suite);
			return NULL;
		}

		if(name)
			strlcpy(suite->name, name, nameLength);

		if(description)
			strlcpy(suite->description, description, descLength);

		suite->run = suite->failed = 0;
		suite->destroyAfterRun = destroyAfterRun;
		suite->firstTest = suite->lastTest = suite->currentTest = NULL;
	}

	return suite;
}

kunit_test_suite_t *kunit_test_suiteCurrent()
{
	return kunit_test_suite_currentSuite;
}

kunit_test_t *kunit_testCurrent()
{
	return kunit_test_suite_currentSuite ? kunit_test_suite_currentSuite->currentTest : NULL;
}

void kunit_test_suiteDestroy(kunit_test_suite_t *suite)
{
	kunit_test_t *test = suite->currentTest;
	while(test)
	{
		kunit_test_t *next = test->next;
		kunit_testDestroy(test);

		test = next;
	}

	if(suite->name)
		hfree(NULL, suite->name);

	if(suite->description)
		hfree(NULL, suite->description);

	hfree(NULL, suite);
}

void kunit_test_suiteAddTest(kunit_test_suite_t *suite, kunit_test_t *test)
{
	assert(test);
	assert(test->suite == NULL);

	test->suite = suite;

	if(suite->lastTest)
	{
		suite->lastTest->next = test;
		suite->lastTest = test;
	}
	else
	{
		suite->firstTest = suite->lastTest = test;
	}
}

void kunit_test_suiteRun(kunit_test_suite_t *suite)
{
	assert(kunit_test_suite_currentSuite == NULL);

	kunit_test_suite_currentSuite = suite;
	info("Test suite '%s' started.\n", suite->name);

	suite->started = time_getTimestamp();

	kunit_test_t *test = suite->firstTest;
	while(test)
	{
		suite->currentTest = test;
		suite->run ++;

		kunit_testRun(test);

		if(test->state == kunit_test_state_failed)
			suite->failed += test->failures;

		test = test->next;
	}

	suite->finished = time_getTimestamp();
	timestamp_t diff = timestamp_getDifference(suite->finished, suite->started);

	uint32_t seconds = timestamp_getSeconds(diff);
	uint32_t millisecs = timestamp_getMilliseconds(diff);

	info("Executed %i tests with %i failures (%i.%03i s).\n\n", suite->run, suite->failed, seconds, millisecs);
	kunit_test_suite_currentSuite = NULL;

	if(suite->destroyAfterRun)
		kunit_test_suiteDestroy(suite);
}
