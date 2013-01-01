//
//  test_atree.c
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

#include <container/atree.h>
#include "unittests.h"

void _test_atree_creation();
void _test_atree_insertion();
void _test_atree_iteration();
void _test_atree_removal();
void _test_atree_find();

void test_atree()
{
	kunit_test_suite_t *atreeSuite = kunit_test_suiteCreate("Atree Tests", "Andersson Tree tests", true);
	{
		kunit_test_suiteAddTest(atreeSuite, kunit_testCreate("Creation test", "Tests wether atrees can be created and initialized", _test_atree_creation));
		kunit_test_suiteAddTest(atreeSuite, kunit_testCreate("Insertion test", "Tests wether data can be inserted into atrees", _test_atree_insertion));
		kunit_test_suiteAddTest(atreeSuite, kunit_testCreate("Iteration test", "Tests wether atrees can be iterators over", _test_atree_iteration));
		kunit_test_suiteAddTest(atreeSuite, kunit_testCreate("Removal test", "Tests wether data can be removed from atrees", _test_atree_removal));
		kunit_test_suiteAddTest(atreeSuite, kunit_testCreate("Find test", "Tests wether data can be found in atrees", _test_atree_find));
	}
	kunit_test_suiteRun(atreeSuite);
}

int _test_atree_comparator(void *key1, void *key2)
{
	uintptr_t tkey1 = (uintptr_t)key1;
	uintptr_t tkey2 = (uintptr_t)key2;

	return (int)(tkey1 - tkey2);
}

void _test_atree_creation()
{
	atree_t *tree = atree_create(_test_atree_comparator);

	KUAssertNotNull(tree, "The tree must not be NULL");
	KUAssertNotNull(tree->root, "The tree must have a root node");
	KUAssertNotNull(tree->nil, "The tree must have a nil node");

	KUAssertEquals(tree->root, tree->nil, "Root and nil node must be equal!");
	KUAssertEquals(atree_count(tree), 0, "The tree must be empty");

	atree_destroy(tree);
}

void _test_atree_insertion()
{
	atree_t *tree = atree_create(_test_atree_comparator);

	for(uint32_t i=0; i<10; i++)
	{
		void *ptr = (void *)(i * 0x100);
		atree_insert(tree, ptr, ptr);
	}

	KUAssertEquals(atree_count(tree), 10, "The tree must have 10 entries");
	atree_destroy(tree);
}

void _test_atree_iteration()
{
	atree_t *tree = atree_create(_test_atree_comparator);

	for(uint32_t i=0; i<10; i++)
	{
		void *ptr = (void *)(i * 0x100);
		atree_insert(tree, ptr, ptr);
	}

	iterator_t *iterator = atree_iterator(tree);
	void *ptr;

	while((ptr = iterator_nextObject(iterator)))
	{
	}

	iterator_destroy(iterator);
	atree_destroy(tree);
}

void _test_atree_removal()
{
	atree_t *tree = atree_create(_test_atree_comparator);

	for(uint32_t i=0; i<10; i++)
	{
		void *ptr = (void *)(i * 0x100);
		atree_insert(tree, ptr, ptr);
	}

	atree_remove(tree, (void *)(0x100));
	atree_remove(tree, (void *)(0x300));
	atree_remove(tree, (void *)(0x600));

	KUAssertEquals(atree_count(tree), 7, "The tree must have 7 entries");
	atree_destroy(tree);
}

void _test_atree_find()
{
		atree_t *tree = atree_create(_test_atree_comparator);

	for(uint32_t i=0; i<10; i++)
	{
		void *ptr = (void *)(i * 0x100);
		atree_insert(tree, ptr, ptr);
	}

	void *find1 = atree_find(tree, (void *)(0x100));
	void *find2 = atree_find(tree, (void *)(0x300));
	void *find3 = atree_find(tree, (void *)(0x600));
	void *find4 = atree_find(tree, (void *)(0x10000));

	KUAssertEquals(find1, (void *)(0x100), "The tree must have find the correct value");
	KUAssertEquals(find2, (void *)(0x300), "The tree must have find the correct value");
	KUAssertEquals(find3, (void *)(0x600), "The tree must have find the correct value");
	KUAssertNull(find4, "The atree must not find non-existend data!");

	atree_destroy(tree);
}

