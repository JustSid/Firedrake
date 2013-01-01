//
//  test_array.c
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

#include <container/array.h>
#include "unittests.h"

void _test_array_creation();
void _test_array_objectAdding();
void _test_array_insertion();
void _test_array_removal();
void _test_array_stressTest();

void test_array()
{
	kunit_test_suite_t *arraySuite = kunit_test_suiteCreate("Array Tests", "Array tests", true);
	{
		kunit_test_suiteAddTest(arraySuite, kunit_testCreate("Creation test", "Tests wether array can be created and initialized", _test_array_creation));
		kunit_test_suiteAddTest(arraySuite, kunit_testCreate("Object adding test", "Tests wether objects can be added to the array", _test_array_objectAdding));
		kunit_test_suiteAddTest(arraySuite, kunit_testCreate("Insertion test", "Tests wether objects can be inserted to the array", _test_array_insertion));
		kunit_test_suiteAddTest(arraySuite, kunit_testCreate("Removal test", "Tests wether objects can be removed from the array", _test_array_removal));
		kunit_test_suiteAddTest(arraySuite, kunit_testCreate("Stress test", "Tests wether the array can handle large number of objects", _test_array_stressTest));
	}
	kunit_test_suiteRun(arraySuite);
}


void _test_array_creation()
{
	array_t *array = array_create();

	KUAssertNotNull(array, "Array must not be NULL.");
	KUAssertNotNull(array->data, "Array must have a backing store.");

	KUAssertEquals(array_count(array), 0, "Arrays must start with a count of 0.");

	array_destroy(array);
}

void _test_array_objectAdding()
{
	array_t *array = array_create();

	for(uint32_t i=0; i<8; i++)
	{
		array_addObject(array, (void *)(0x100 * i));
	}

	KUAssertEquals(array_count(array), 8, "The array must have 8 objects in it!.");

	for(uint32_t i=0; i<8; i++)
	{
		void *pointer = (void *)(0x100 * i);
		void *test = array_objectAtIndex(array, i);

		KUAssertEquals(pointer, test, "The pointer in the array must be the same as stored in it");
	}

	array_destroy(array);
}

void _test_array_insertion()
{
	array_t *array = array_create();

	for(uint32_t i=0; i<8; i++)
	{
		array_addObject(array, (void *)(0x100 * i));
	}

	KUAssertEquals(array_count(array), 8, "The array must have 8 objects in it!.");

	for(uint32_t i=0; i<5; i++)
	{
		array_insertObject(array, (void *)0x111, 0);
		array_insertObject(array, (void *)0x999, 6);
	}

	KUAssertEquals(array_count(array), 18, "The array must have 18 objects in it!.");


	KUAssertEquals(array_objectAtIndex(array, 0), (void *)0x111, "The stored value must match what was put into the array.");
	KUAssertEquals(array_objectAtIndex(array, 1), (void *)0x111, "The stored value must match what was put into the array.");
	KUAssertEquals(array_objectAtIndex(array, 2), (void *)0x111, "The stored value must match what was put into the array.");
	KUAssertEquals(array_objectAtIndex(array, 3), (void *)0x111, "The stored value must match what was put into the array.");
	KUAssertEquals(array_objectAtIndex(array, 4), (void *)0x111, "The stored value must match what was put into the array.");

	KUAssertEquals(array_objectAtIndex(array, 6), (void *)0x999, "The stored value must match what was put into the array.");
	KUAssertEquals(array_objectAtIndex(array, 8), (void *)0x999, "The stored value must match what was put into the array.");
	KUAssertEquals(array_objectAtIndex(array, 10), (void *)0x999, "The stored value must match what was put into the array.");
	KUAssertEquals(array_objectAtIndex(array, 12), (void *)0x999, "The stored value must match what was put into the array.");
	KUAssertEquals(array_objectAtIndex(array, 14), (void *)0x999, "The stored value must match what was put into the array.");

	array_destroy(array);
}

void _test_array_removal()
{
	array_t *array = array_create();

	for(uint32_t i=0; i<8; i++)
	{
		array_addObject(array, (void *)(0x100 * i));
	}

	array_removeObjectAtIndex(array, 0);
	array_removeObjectAtIndex(array, 6);
	array_removeObjectAtIndex(array, 2);

	KUAssertEquals(array_count(array), 5, "The array must have 5 objects in it!.");

	/*for(uint32_t i=0; i<array_count(array); i++)
	{
		void *ptr = array_objectAtIndex(array, i);
		dbg("%i: %p\n", i, ptr);
	}*/

	KUAssertEquals(array_objectAtIndex(array, 0), (void *)0x100, "The stored value must match what was put into the array.");
	KUAssertEquals(array_objectAtIndex(array, 1), (void *)0x200, "The stored value must match what was put into the array.");
	KUAssertEquals(array_objectAtIndex(array, 2), (void *)0x400, "The stored value must match what was put into the array.");
	KUAssertEquals(array_objectAtIndex(array, 3), (void *)0x500, "The stored value must match what was put into the array.");
	KUAssertEquals(array_objectAtIndex(array, 4), (void *)0x600, "The stored value must match what was put into the array.");

	array_removeAllObjects(array);
	KUAssertEquals(array_count(array), 0, "The array must be empty!");

	array_destroy(array);
}

#define kArrayStressTestCount 10000

void _test_array_stressTest()
{
	array_t *array = array_create();

	for(uint32_t iteration=0; iteration<10; iteration++)
	{
		for(uint32_t i=0; i<kArrayStressTestCount; i++)
		{
			array_addObject(array, (void *)(0x100 * i));
		}

		KUAssertEquals(array_count(array), kArrayStressTestCount, "The array must have %i objects in it!.", kArrayStressTestCount);

		for(uint32_t i=0; i<5; i++)
		{
			for(uint32_t j=0; j<array_count(array); j++)
			{
				if((j % 2) == 0)
				{
					array_removeObjectAtIndex(array, j);
				}
			}
		}

		array_removeAllObjects(array);
	}

	array_destroy(array);
}

