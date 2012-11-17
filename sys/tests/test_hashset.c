//
//  test_hashset.c
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

#include <container/hashset.h>
#include <memory/memory.h>
#include <libc/stdio.h>
#include "unittests.h"

void _test_hashset_creation();
void _test_hashset_insertion();
void _test_hashset_deletion();
void _test_hashset_lookup();
void _test_hashset_stressTest();

void test_hashset()
{
	kunit_test_suite_t *hashsetSuite = kunit_test_suiteCreate("Hashset Tests", "Hashset tests", true);
	{
		kunit_test_suiteAddTest(hashsetSuite, kunit_testCreate("Creation test", "Tests wether hashsets can be created and initialized", _test_hashset_creation));
		kunit_test_suiteAddTest(hashsetSuite, kunit_testCreate("Insertion test", "Tests wether data can be added to hashsets", _test_hashset_insertion));
		kunit_test_suiteAddTest(hashsetSuite, kunit_testCreate("Deletion test", "Tests wether entries can be removed from hashsets", _test_hashset_deletion));
		kunit_test_suiteAddTest(hashsetSuite, kunit_testCreate("Lookup test", "Tests wether lookups are working", _test_hashset_lookup));
		kunit_test_suiteAddTest(hashsetSuite, kunit_testCreate("Stress test", "Stess tests the hashset", _test_hashset_stressTest));
	}
	kunit_test_suiteRun(hashsetSuite);
}

void _test_hashset_creation()
{
	hashset_t *set = hashset_create(0, hash_cstring);

	KUAssertNotNull(set, "The hashset must not be NULL");
	KUAssertNotNull(set->buckets, "The hashset must have buckets!");
	KUAssertTrue((set->capacity > 0), "The hashset must have a capacity greater 0!"); 
	KUAssertEquals(hashset_count(set), 0, "The hashset mut be empty!");

	hashset_destroy(set);
}

void _test_hashset_insertion()
{
	hashset_t *set = hashset_create(0, hash_cstring);

	hashset_setDataForKey(set, (void *)(0x100), "Key1");
	hashset_setDataForKey(set, (void *)(0x200), "Key2");
	hashset_setDataForKey(set, (void *)(0x300), "Key3");

	KUAssertEquals(hashset_count(set), 3, "The hashset must have 3 entries!");

	hashset_destroy(set);
}

void _test_hashset_deletion()
{
	hashset_t *set = hashset_create(0, hash_cstring);

	hashset_setDataForKey(set, (void *)(0x100), "Key1");
	hashset_setDataForKey(set, (void *)(0x200), "Key2");
	hashset_setDataForKey(set, (void *)(0x300), "Key3");
	hashset_setDataForKey(set, (void *)(0x400), "Key4");
	hashset_setDataForKey(set, (void *)(0x500), "Key5");
	hashset_setDataForKey(set, (void *)(0x600), "Key6");
	hashset_setDataForKey(set, (void *)(0x700), "Key7");

	hashset_removeDataForKey(set, "Key2");
	hashset_removeDataForKey(set, "Key7");
	hashset_removeDataForKey(set, "Key3");

	KUAssertEquals(hashset_count(set), 4, "The hashset must have 4 entries!");

	KUAssertNull(hashset_dataForKey(set, "Key2"), "The data must be NULL!");
	KUAssertNull(hashset_dataForKey(set, "Key7"), "The data must be NULL!");
	KUAssertNull(hashset_dataForKey(set, "Key3"), "The data must be NULL!");

	hashset_destroy(set);
}

void _test_hashset_lookup()
{
	hashset_t *set = hashset_create(0, hash_cstring);

	hashset_setDataForKey(set, (void *)(0x100), "Key1");
	hashset_setDataForKey(set, (void *)(0x200), "Key2");
	hashset_setDataForKey(set, (void *)(0x300), "Key3");
	hashset_setDataForKey(set, (void *)(0x400), "Key4");
	hashset_setDataForKey(set, (void *)(0x500), "Key5");
	hashset_setDataForKey(set, (void *)(0x600), "Key6");
	hashset_setDataForKey(set, (void *)(0x700), "Key7");

	KUAssertEquals(hashset_count(set), 7, "The hashset must have 7 entries!");

	KUAssertEquals(hashset_dataForKey(set, "Key1"), (void *)(0x100), "The data must be equal!");
	KUAssertEquals(hashset_dataForKey(set, "Key2"), (void *)(0x200), "The data must be equal!");
	KUAssertEquals(hashset_dataForKey(set, "Key3"), (void *)(0x300), "The data must be equal!");
	KUAssertEquals(hashset_dataForKey(set, "Key4"), (void *)(0x400), "The data must be equal!");
	KUAssertEquals(hashset_dataForKey(set, "Key5"), (void *)(0x500), "The data must be equal!");
	KUAssertEquals(hashset_dataForKey(set, "Key6"), (void *)(0x600), "The data must be equal!");
	KUAssertEquals(hashset_dataForKey(set, "Key7"), (void *)(0x700), "The data must be equal!");

	hashset_destroy(set);
}

#define kHashsetStressTestCount 10000

void _test_hashset_stressTest()
{
	for(uint32_t iteration=0; iteration<10; iteration++)
	{
		hashset_t *set = hashset_create(0, hash_cstring);
		heap_t *heap = heap_create(kHeapFlagSecure | kHeapFlagAligned);

		for(uint32_t i=0; i<kHashsetStressTestCount; i++)
		{
			char *string = halloc(heap, 16);
			sprintf(string, "key %i", i);

			hashset_setDataForKey(set, (void *)((i + 0x100) * 0x100), string);
		}

		KUAssertEquals(hashset_count(set), kHashsetStressTestCount, "The hashset must contain %i entries!", kHashsetStressTestCount);

		heap_destroy(heap);
		hashset_destroy(set);
	}
}
