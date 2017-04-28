/* vim:set sw=8 ts=8 noet: */
/*
 * Copyright (c) 2016-2017 Torchbox Ltd.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely. This software is provided 'as-is', without any express or implied
 * warranty.
 */

/*
 * Tests for hash.c: simple FNV-1a-based hash table.
 */

#include	<string>
#include	<vector>
#include	<map>

#include	"hash.h"

#include	"gtest/gtest.h"

using std::vector;
using std::map;
using std::string;

namespace {
	/*
	 * Repeat each test with various different hash sizes, to exercise both the
	 * hashing and the bucket handling code.
	 */
	vector<int>	test_sizes{ 1, 2, 7, 127, 15601 };

} // anonymous namespace

#define	VOID_P(s)	const_cast<void *>(static_cast<void const *>(s))

/*
 * Test various primitive hash functions.  Each test inserts 6 keys, then
 * deletes three of them; this is intended to exercise the single linked list
 * deletion code.
 *
 * Unfortunately the hash interface is not especially amenable to use in C++,
 * the a lot of casting is required to use it.
 */

TEST(Hash, SetGet)
{
	for (int size: test_sizes) {
	hash_t	hs;
	char	*s;

		hs = hash_new(size, NULL);

		hash_set(hs, "delete 1", VOID_P("x"));
		hash_set(hs, "foo", VOID_P("foo key"));
		hash_set(hs, "delete 2", VOID_P("x"));
		hash_set(hs, "bar", VOID_P("bar key"));
		hash_del(hs, "delete 2");
		hash_set(hs, "quux", VOID_P("quux key"));
		hash_set(hs, "delete 3", VOID_P("x"));
		hash_del(hs, "delete 1");
		hash_del(hs, "delete 3");

		s = static_cast<char *>(hash_get(hs, "foo"));
		ASSERT_NE(s, static_cast<char *>(NULL));
		EXPECT_EQ(0, strcmp(s, "foo key"));

		s = static_cast<char *>(hash_get(hs, "bar"));
		ASSERT_NE(s, static_cast<char *>(NULL));
		EXPECT_EQ(0, strcmp(s, "bar key"));

		s = static_cast<char *>(hash_get(hs, "quux"));
		ASSERT_NE(s, static_cast<char *>(NULL));
		EXPECT_EQ(0, strcmp(s, "quux key"));

		hash_free(hs);
	}
}

namespace {

void
foreach_incr(hash_t hs, const char *key, void *value, void *data)
{
	++*static_cast<int *>(data);
}

} // anonymous namespace

TEST(Hash, Foreach)
{
	for (int size: test_sizes) {
	hash_t	hs;
		hs = hash_new(size, NULL);

		hash_set(hs, "delete 1", VOID_P("x"));
		hash_set(hs, "foo", VOID_P("foo key"));
		hash_set(hs, "delete 2", VOID_P("x"));
		hash_set(hs, "bar", VOID_P("bar key"));
		hash_del(hs, "delete 2");
		hash_set(hs, "quux", VOID_P("quux key"));
		hash_set(hs, "delete 3", VOID_P("x"));
		hash_del(hs, "delete 1");
		hash_del(hs, "delete 3");

		int i = 0;
		hash_foreach(hs, foreach_incr, &i);

		EXPECT_EQ(i, 3);

		hash_free(hs);
	}
}

TEST(Hash, Iterate)
{
	for (int size: test_sizes) {
	struct hash_iter_state	 iterstate;
	hash_t			 hs = NULL;
	const char		*k = NULL;
	void			*v = NULL;
	int			 i = 0;
	map<string, string>	 actual, expected{
			{ "foo", "foo key" },
			{ "bar", "bar key" },
			{ "quux", "quux key" },
		};

		hs = hash_new(size, NULL);

		hash_set(hs, "delete 1", VOID_P("x"));
		hash_set(hs, "foo", VOID_P("foo key"));
		hash_set(hs, "delete 2", VOID_P("x"));
		hash_set(hs, "bar", VOID_P("bar key"));
		hash_del(hs, "delete 2");
		hash_set(hs, "quux", VOID_P("quux key"));
		hash_set(hs, "delete 3", VOID_P("x"));
		hash_del(hs, "delete 1");
		hash_del(hs, "delete 3");

		bzero(&iterstate, sizeof(iterstate));

		/*
		 * This test relies on a specific iteration order.  Any change
		 * in the hash function will required that the test is changed
		 * to match.
		 */

		for (int c = 0; c < 3; c++) {
			i = hash_iterate(hs, &iterstate, &k, &v);
			ASSERT_EQ(i, 1);
			actual.emplace(k, static_cast<char const *>(v));
		}

		i = hash_iterate(hs, &iterstate, &k, &v);
		EXPECT_EQ(i, 0);

		EXPECT_EQ(expected, actual);

		hash_free(hs);
	}
}
