/*
 * Phoenix-RTOS
 *
 *    POSIX.1-2017 standard library functions tests
 *    HEADER:
 *    - stdlib.h
 *    TESTED:
 *    - bsearch()
 *
 * Copyright 2023 Phoenix Systems
 * Author: Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <unity_fixture.h>


struct node {
	const char *string;
	int length;
};

/* Special node with reversed attributes */
struct node2 {
	int length;
	const char *string;
};


/* table sorted alphabetically */
static const char *const test_names[] = { "adam",
	"bartek",
	"cezary",
	"damian",
	"edward",
	"fryderyk",
	"gracjan",
	"henryk",
	"iwo",
	"jan" };

static const char *const namesRepeat[] = { "adam",
	"adam",
	"adam",
	"damian",
	"edward",
	"fryderyk",
	"fryderyk",
	"henryk",
	"henryk",
	"jan" };

/* table sorted alphabetically */
static const char *const names_nsort[] = { "henryk",
	"bartek",
	"cezary",
	"damian",
	"edward",
	"fryderyk",
	"gracjan",
	"adam",
	"iwo",
	"jan" };

#define TABSIZE (sizeof(test_names) / sizeof(test_names[0]))


static void test_prepare(const char *const names_arg[], struct node table[])
{
	int i;

	for (i = 0; i < TABSIZE; i++) {
		table[i].string = names_arg[i];
		table[i].length = strlen(names_arg[i]);
	}
}


/* bsearch can't change content of table, but may reorder elements */
static int test_checkTab(const char *const names_arg[], struct node table[])
{
	int i, j, found;
	int found_tab[TABSIZE];
	memset(found_tab, 0, sizeof(int) * TABSIZE);

	for (i = 0; i < TABSIZE; i++) {
		found = 0;
		for (j = 0; j < TABSIZE && found_tab[i] == 0; j++) {
			if (strcmp(names_arg[i], table[j].string) == 0 && table[i].length == table[j].length) {
				found = 1;
				found_tab[i] = 1;
				break;
			}
		}
		if (found == 0) {
			return found;
		}
	}

	return found;
}


/* compare function used by bsearch */
static int test_nodeCmpStr(const void *node1, const void *node2)
{
	return strcmp(((struct node *)node1)->string, ((struct node *)node2)->string);
}


/* compare function used by bsearch in special_node testcase */
static int test_nodeCmpLen(const void *node1, const void *node2)
{
	if (((struct node *)node1)->length == ((struct node *)node2)->length) {
		return 0;
	}
	else {
		return 1;
	}
}


TEST_GROUP(stdlib_bsearch);


TEST_SETUP(stdlib_bsearch)
{
}

TEST_TEAR_DOWN(stdlib_bsearch)
{
}


TEST(stdlib_bsearch, zero_elements)
{
	struct node table[TABSIZE];
	struct node *rval, node;
	node.string = "adam";
	node.length = strlen(node.string);

	test_prepare(test_names, table);

	rval = (struct node *)bsearch((void *)&node, (void *)table,
		0, sizeof(struct node), test_nodeCmpStr);
	TEST_ASSERT_NULL(rval);
}


TEST(stdlib_bsearch, find_node_in)
{
	int i;
	struct node table[TABSIZE];
	struct node *rval, node;

	/* Check for even number of elements */
	for (i = 0; i < TABSIZE; i++) {
		test_prepare(test_names, table);
		node.string = test_names[i];
		node.length = strlen(test_names[i]);
		rval = (struct node *)bsearch((void *)&node, (void *)table,
			(size_t)TABSIZE, sizeof(struct node), test_nodeCmpStr);
		TEST_ASSERT_NOT_NULL(rval);
		TEST_ASSERT_EQUAL_MEMORY(&table[i], rval, sizeof(*rval));

		TEST_ASSERT_EQUAL_INT(1, test_checkTab(test_names, table));
	}

	/* Check for odd number of elements */
	for (i = 0; i < TABSIZE - 1; i++) {
		test_prepare(test_names, table);
		node.string = test_names[i];
		node.length = strlen(test_names[i]);
		rval = (struct node *)bsearch((void *)&node, (void *)table,
			(size_t)TABSIZE - 1, sizeof(struct node), test_nodeCmpStr);
		TEST_ASSERT_NOT_NULL(rval);
		TEST_ASSERT_EQUAL_MEMORY(&table[i], rval, sizeof(*rval));

		TEST_ASSERT_EQUAL_INT(1, test_checkTab(test_names, table));
	}
}


TEST(stdlib_bsearch, find_node_not_in)
{
	struct node table[TABSIZE];
	struct node *rval, node;
	char name[] = "nonPresentName";
	node.string = name;
	node.length = strlen(name);

	/* Check for even number of elements */
	test_prepare(test_names, table);

	rval = (struct node *)bsearch((void *)&node, (void *)table,
		(size_t)TABSIZE, sizeof(struct node), test_nodeCmpStr);
	TEST_ASSERT_NULL(rval);

	TEST_ASSERT_EQUAL_INT(1, test_checkTab(test_names, table));

	/* Check for odd number of elements */
	test_prepare(test_names, table);

	rval = (struct node *)bsearch((void *)&node, (void *)table,
		(size_t)TABSIZE - 1, sizeof(struct node), test_nodeCmpStr);
	TEST_ASSERT_NULL(rval);

	TEST_ASSERT_EQUAL_INT(1, test_checkTab(test_names, table));
}


TEST(stdlib_bsearch, not_sorted)
{
	/* Elements are not sorted in this way that bsearch won't find searched element */
	struct node table[TABSIZE];
	struct node *rval, node;
	char name[] = "adam";
	node.string = name;
	node.length = strlen(name);

	/* Check for even number of elements */
	test_prepare(names_nsort, table);

	rval = (struct node *)bsearch((void *)&node, (void *)table,
		(size_t)TABSIZE, sizeof(struct node), test_nodeCmpStr);
	TEST_ASSERT_NULL(rval);

	TEST_ASSERT_EQUAL_INT(1, test_checkTab(names_nsort, table));

	/* Check for odd number of elements */
	test_prepare(names_nsort, table);

	rval = (struct node *)bsearch((void *)&node, (void *)table,
		(size_t)TABSIZE - 1, sizeof(struct node), test_nodeCmpStr);
	TEST_ASSERT_NULL(rval);

	TEST_ASSERT_EQUAL_INT(1, test_checkTab(names_nsort, table));
}

TEST(stdlib_bsearch, same_members)
{
	struct node table[TABSIZE];
	struct node *rval, node;
	char name[] = "adam";
	node.string = name;
	node.length = strlen(name);

	/* Check for even number of elements */
	test_prepare(namesRepeat, table);

	rval = (struct node *)bsearch((void *)&node, (void *)table,
		(size_t)TABSIZE, sizeof(struct node), test_nodeCmpStr);
	TEST_ASSERT_NOT_NULL(rval);
	TEST_ASSERT_EQUAL_STRING(node.string, rval->string);
	TEST_ASSERT_EQUAL_INT(node.length, rval->length);

	TEST_ASSERT_EQUAL_INT(1, test_checkTab(namesRepeat, table));

	/* Check for odd number of elements */
	test_prepare(namesRepeat, table);

	rval = (struct node *)bsearch((void *)&node, (void *)table,
		(size_t)TABSIZE - 1, sizeof(struct node), test_nodeCmpStr);
	TEST_ASSERT_NOT_NULL(rval);
	TEST_ASSERT_EQUAL_STRING(node.string, rval->string);
	TEST_ASSERT_EQUAL_INT(node.length, rval->length);

	TEST_ASSERT_EQUAL_INT(1, test_checkTab(namesRepeat, table));
}


TEST(stdlib_bsearch, special_node)
{
	/* test asserts that if key node has same members values but not in the same order bsearch won't find */
	struct node table[TABSIZE];
	struct node *rval;
	struct node2 node2;
	char name[] = "adam";
	node2.string = name;
	node2.length = strlen(name);

	test_prepare(test_names, table);

	rval = (struct node *)bsearch((void *)&node2, (void *)table,
		(size_t)TABSIZE, sizeof(struct node), test_nodeCmpLen);
	TEST_ASSERT_NULL(rval);

	TEST_ASSERT_EQUAL_INT(1, test_checkTab(test_names, table));
}


TEST_GROUP_RUNNER(stdlib_bsearch)
{
	RUN_TEST_CASE(stdlib_bsearch, zero_elements);
	RUN_TEST_CASE(stdlib_bsearch, find_node_in);
	RUN_TEST_CASE(stdlib_bsearch, find_node_not_in);
	RUN_TEST_CASE(stdlib_bsearch, not_sorted);
	RUN_TEST_CASE(stdlib_bsearch, same_members);
	RUN_TEST_CASE(stdlib_bsearch, special_node);
}
