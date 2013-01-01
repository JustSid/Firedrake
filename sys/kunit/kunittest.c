//
//  kunittest.c
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2013 by Sidney Just
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

#include <config.h>
#include <libc/string.h>
#include <libc/stdio.h>
#include <system/syslog.h>
#include <memory/memory.h>

#include "kunittest.h"

kunit_test_t *kunit_testCreate(char *name, char *description, kunit_test_function_t function)
{
	if(!function)
	{
		warn("kunit_testCreate() with NULL function!");
		return NULL;
	}

	kunit_test_t *test = halloc(NULL, sizeof(kunit_test_t));
	if(test)
	{
		size_t nameLength = name ? strlen(name) : 0;
		size_t descLength = description ? strlen(description) : 0;

		test->name = name ? halloc(NULL, nameLength + 1) : NULL;
		test->description = description ? halloc(NULL, descLength + 1) : NULL;

		if((test->name == NULL && name) || (test->description == NULL && description))
		{
			if(test->name)
				hfree(NULL, test->name);

			if(test->description)
				hfree(NULL, test->description);

			hfree(NULL, test);
			return NULL;
		}

		if(name)
			strlcpy(test->name, name, nameLength);

		if(description)
			strlcpy(test->description, description, descLength);


		test->state = kunit_test_state_waiting;
		test->suite = NULL;
		test->continueOnError = true;
		test->failures = 0;
		test->buffer = NULL;
		test->function = function;
		test->next = NULL;
	}

	return test;
}

void kunit_testDestroy(kunit_test_t *test)
{
	if(test->name)
		hfree(NULL, test->name);

	if(test->description)
		hfree(NULL, test->description);

	hfree(NULL, test);
}



void kunit_testRun(kunit_test_t *test)
{
	jmp_buf buffer;
	test->buffer = &buffer;

	test->started = time_getTimestamp();

	if(setjmp(buffer) == 0)
	{
#if CONF_KUNITFAILSONLY
		info("Test case '%s' started.\n", test->name);
#endif

		test->state = kunit_test_state_ran;
		test->function();
	}

	test->finished = time_getTimestamp();
	timestamp_t diff = test->finished - test->started;


	if(test->state == kunit_test_state_ran)
	{
#if CONF_KUNITFAILSONLY
		info("Test case '%s' passed (%i.%03i s).\n", test->name, time_getSeconds(diff), time_getMilliseconds(diff));
#endif
	}
	else
	{
		info("Test case '%s' failed (%i.%03i s).\n", test->name, time_getSeconds(diff), time_getMilliseconds(diff));
	}

	test->buffer = NULL;
}

void kunit_testFail(kunit_test_t *test, char *UNUSED(description), ...)
{
	test->state = kunit_test_state_failed;
	test->failures ++;

	if(!test->continueOnError && test->buffer)
		longjmp(*test->buffer, 0);
}
