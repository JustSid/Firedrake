//
//  test_kunit.c
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

#include "unittests.h"

void _test_kunit_simpleSuitTest();
void _test_kunit_NullSuiteTest();
void _test_kunit_AssertionTest();
void _test_kunit_TestMember();
void _test_kunit_JumpOnFail();

void test_kunit()
{
	kunit_test_suite_t *testSuite = kunit_test_suiteCreate("Test Suite", "A test suite which creates and tests test suites", true);
	{
		kunit_test_suiteAddTest(testSuite, kunit_testCreate("Simple test", "Tests wether test suites can be created or not", _test_kunit_simpleSuitTest));
		kunit_test_suiteAddTest(testSuite, kunit_testCreate("NULL test", "Tests wether test suites can be created with NULL arguments", _test_kunit_NullSuiteTest));
		kunit_test_suiteAddTest(testSuite, kunit_testCreate("Assertion test", "Tests wether assertions work correctly", _test_kunit_AssertionTest));
	}
	kunit_test_suiteRun(testSuite);

	// ---
	kunit_test_suite_t *testsSuite = kunit_test_suiteCreate("Tests Suite", "Tests if kunit_test_t is working correctly", true);
	{
		kunit_test_suiteAddTest(testsSuite, kunit_testCreate("Member test", "Tests the members of kunit_test_t", _test_kunit_TestMember));
		kunit_test_suiteAddTest(testsSuite, kunit_testCreate("Jump test", "Tests wether jumps on failures work", _test_kunit_JumpOnFail));
	}
	kunit_test_suiteRun(testsSuite);
}




void _test_kunit_simpleSuitTest()
{
	kunit_test_suite_t *suite = kunit_test_suiteCreate("Test", "A test suite", true);

	KUAssertNotNull(suite, "The suite must not be NULL!"); // When Unit Tests are run, there IS enough memory to allocate a suite!
	KUAssertNotNull(suite->name, "The name of the suite must not be NULL!");
	KUAssertNotNull(suite->description, "The description of the suite must not be NULL!");

	KUAssertEquals(suite->run, 0, "The suite must start with 0 run tests!");
	KUAssertEquals(suite->failed, 0, "The suite must start with 0 failed tests!");

	kunit_test_suiteDestroy(suite);
}

void _test_kunit_NullSuiteTest()
{
	kunit_test_t *test = kunit_testCurrent();
	test->continueOnError = false;

	kunit_test_suite_t *noName = kunit_test_suiteCreate(NULL, "A suite without a name", true);
	kunit_test_suite_t *noDesc = kunit_test_suiteCreate("No description", NULL, true);

	KUAssertNotNull(noName, "Test suite must not be NULL!");
	KUAssertNotNull(noDesc, "Test suite must not be NULL!");

	KUAssertNull(noName->name, NULL);
	KUAssertNull(noDesc->description, NULL);

	kunit_test_suiteDestroy(noName);
	kunit_test_suiteDestroy(noDesc);	
}

void _test_kunit_AssertionTest()
{
	kunit_test_t *test = kunit_testCurrent();
	test->continueOnError = false;

	// NONE of these should fail!
	KUAssertEquals(1, 1, NULL);
	KUAssertEquals('c', 'c', NULL);

	KUAssertEqualsWithAccuracy(1.0, 1.0, 0.0001, NULL);
	KUAssertEqualsWithAccuracy(2.3 - 1.3, 1.0, 0.0001, NULL);
	KUAssertEqualsWithAccuracy(10.3582 - 9.23151, 1.0, 0.2, NULL);

	KUAssertNull(NULL, NULL);
	KUAssertNotNull((void *)0x1000, NULL);

	KUAssertTrue(1, NULL);
	KUAssertTrue(0 + 22, NULL);

	KUAssertFalse(0, NULL);
	KUAssertFalse(NULL, NULL);

	// Lets test if tests can fail, thus we need the continue on error functionality!
	test->continueOnError = true;

	warn("All of the following 6 assertions MUST fail!\n");

	KUAssertEquals(0, 1, NULL);
	KUAssertEqualsWithAccuracy(10.3582 - 9.23151, 1.0, 0.001, NULL);
	KUAssertNull((void *)0x1000, NULL);
	KUAssertNotNull(NULL, NULL);
	KUAssertTrue(false, NULL);
	KUAssertFalse(true, NULL);


	// Reset the failure state because produced failures are actually successes in the above cases
	test->state = kunit_test_state_ran;

	// Lets check if there were 6 failures and thus only fail the test case if there weren't
	KUAssertEquals(test->failures, 6, NULL);
}

void _test_kunit_TestMember()
{
	kunit_test_t *test = kunit_testCurrent();

	KUAssertEquals(test->state, kunit_test_state_ran, "The current tests state should be running!");
	KUAssertEquals(test->failures, 0, "The current test shouldn't have any failures");
	KUAssertNotNull(test->buffer, "The current test should have a jump buffer!");
	KUAssertEquals(test->suite, kunit_test_suiteCurrent(), "The current tests suite should be equal to the currently running suite");
}

void _test_kunit_JumpOnFail()
{
	kunit_test_t *test = kunit_testCurrent();
	test->continueOnError = false;

	warn("This test is supposed to fail!\n");
	KUAssertTrue(false, NULL); // MUST fail here!

	KUFail("We MUST not be here!");
}
