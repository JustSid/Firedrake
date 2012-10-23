//
//  kunit.h
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

/**
 * Overview:
 * Defines a simple unit test interface for the kernel, actually just a bunch of macros
 **/
#ifndef _KUNIT_H_
#define _KUNIT_H_

#include <system/panic.h>
#include <system/syslog.h>
#include <system/helper.h>
#include <libc/math.h>

#include "kunittest.h"
#include "kunittestsuite.h"

#define KUFail(...) \
	do { \
		kunit_test_t *__ktest = kunit_testCurrent(); \
		if(__ktest) \
		{ \
			kunit_testFail(__ktest, __VA_ARGS__); \
		} \
		else \
		{ \
			err("kunit test failed!\n"); \
			panic(__VA_ARGS__); \
		} \
	} while(0)

#define KUAssertEquals(a1, a2, ...) \
	do { \
		if(a1 != a2) \
		{ \
			kunit_test_t *__ktest = kunit_testCurrent(); \
			if(__ktest) \
			{ \
				err("%s:%i: Expected (%s) == (%s)\n", sys_fileWithoutPath(__FILE__), __LINE__, #a1, #a2); \
				kunit_testFail(__ktest, __VA_ARGS__); \
			} \
			else \
			{ \
				err("%s:%i: Expected (%s) == (%s)\n", sys_fileWithoutPath(__FILE__), __LINE__, #a1, #a2); \
				panic(__VA_ARGS__); \
			} \
		} \
	} while(0)

#define KUAbsoluteDifference(left, right) (MAX(left, right) - MIN(left, right))

#define KUAssertEqualsWithAccuracy(a1, a2, accuracy, ...) \
	do { \
		if(KUAbsoluteDifference((a1), (a2)) > accuracy) \
		{ \
			kunit_test_t *__ktest = kunit_testCurrent(); \
			if(__ktest) \
			{ \
				err("%s:%i: Expected (%s) - (%s) <= (%s)\n", sys_fileWithoutPath(__FILE__), __LINE__, #a1, #a2, #accuracy); \
				kunit_testFail(__ktest, __VA_ARGS__); \
			} \
			else \
			{ \
				err("%s:%i: Expected (%s) - (%s) <= (%s)\n", sys_fileWithoutPath(__FILE__), __LINE__, #a1, #a2, #accuracy); \
				panic(__VA_ARGS__); \
			} \
		} \
	} while(0)

#define KUAssertNull(a1, ...) \
	do { \
		if(a1 != NULL) \
		{ \
			kunit_test_t *__ktest = kunit_testCurrent(); \
			if(__ktest) \
			{ \
				err("%s:%i: Expected (%s) to be NULL!\n", sys_fileWithoutPath(__FILE__), __LINE__, #a1); \
				kunit_testFail(__ktest, __VA_ARGS__); \
			} \
			else \
			{ \
				err("%s:%i: Expected (%s) to be NULL!\n", sys_fileWithoutPath(__FILE__), __LINE__, #a1); \
				panic(__VA_ARGS__); \
			} \
		} \
	} while(0)

#define KUAssertNotNull(a1, ...) \
	do { \
		if(a1 == NULL) \
		{ \
			kunit_test_t *__ktest = kunit_testCurrent(); \
			if(__ktest) \
			{ \
				err("%s:%i: Expected (%s) to be not NULL!\n", sys_fileWithoutPath(__FILE__), __LINE__, #a1); \
				kunit_testFail(__ktest, __VA_ARGS__); \
			} \
			else \
			{ \
				err("%s:%i: Expected (%s) to be not NULL!\n", sys_fileWithoutPath(__FILE__), __LINE__, #a1); \
				panic(__VA_ARGS__); \
			} \
		} \
	} while(0)

#define KUAssertTrue(expr, ...) \
	do { \
		if(!(expr)) \
		{ \
			kunit_test_t *__ktest = kunit_testCurrent(); \
			if(__ktest) \
			{ \
				err("%s:%i: Expected (%s) to be true!\n", sys_fileWithoutPath(__FILE__), __LINE__, #expr); \
				kunit_testFail(__ktest, __VA_ARGS__); \
			} \
			else \
			{ \
				err("%s:%i: Expected (%s) to be true!\n", sys_fileWithoutPath(__FILE__), __LINE__, #expr); \
				panic(__VA_ARGS__); \
			} \
		} \
	} while(0)

#define KUAssertFalse(expr, ...) \
	do { \
		if((expr)) \
		{ \
			kunit_test_t *__ktest = kunit_testCurrent(); \
			if(__ktest) \
			{ \
				err("%s:%i: Expected (%s) to be false!\n", sys_fileWithoutPath(__FILE__), __LINE__, #expr); \
				kunit_testFail(__ktest, __VA_ARGS__); \
			} \
			else \
			{ \
				err("%s:%i: Expected (%s) to be false!\n", sys_fileWithoutPath(__FILE__), __LINE__, #expr); \
				panic(__VA_ARGS__); \
			} \
		} \
	} while(0)

#endif /* _KUNIT_H_ */
