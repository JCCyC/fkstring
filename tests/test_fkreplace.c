#include <string.h>
#include <fkstring.h>
#include "framework.h"

static int test_fkreplace_null_args(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc"), *a = fkstrnew("a"), *b = fkstrnew("b");

	CHECK(fkreplace(NULL, a, b, 0) == NULL, "fkreplace(NULL, ...) should return NULL");
	CHECK(fkreplace(s, NULL, b, 0) == NULL, "NULL old should return NULL");
	CHECK(fkreplace(s, a, NULL, 0) == NULL, "NULL new should return NULL");
	CHECK(fkreplacec(NULL, "a", "b", 0) == NULL, "fkreplacec(NULL, ...) should return NULL");
	CHECK(fkreplacec(s, NULL, "b", 0) == NULL, "NULL old should return NULL");
	CHECK(fkreplacec(s, "a", NULL, 0) == NULL, "NULL new should return NULL");
	CHECK(strcmp(fkcstr(s), "abc") == 0, "fks modified: '%s'", fkcstr(s));
	fkstrdestroy(s);
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkreplace_all(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("a-b-c-d");

	CHECK(fkreplacec(s, "-", ", ", 0) == s, "expected fks returned for chaining");
	CHECK(strcmp(fkcstr(s), "a, b, c, d") == 0, "expected 'a, b, c, d', got '%s'", fkcstr(s));
	CHECK(fkstrlen(s) == 10, "expected len 10, got %zu", fkstrlen(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkreplace_max_count(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("xxxxx");

	fkreplacec(s, "x", "y", 2);
	CHECK(strcmp(fkcstr(s), "yyxxx") == 0, "max_count 2: got '%s'", fkcstr(s));
	fkreplacec(s, "x", "y", 100);
	CHECK(strcmp(fkcstr(s), "yyyyy") == 0, "max_count > matches: got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkreplace_no_match(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("hello");
	char *oldbuf = fkcstr(s);
	size_t oldalloc = fkstrsize(s);

	CHECK(fkreplacec(s, "z", "yy", 0) == s, "expected fks returned");
	CHECK(fkcstr(s) == oldbuf && fkstrsize(s) == oldalloc, "no match should not reallocate");
	CHECK(strcmp(fkcstr(s), "hello") == 0, "got '%s'", fkcstr(s));
	CHECK(fkreplacec(s, "hello!", "x", 0) == s && strcmp(fkcstr(s), "hello") == 0,
	      "old longer than fks should not match");
	fkstrdestroy(s);
	return 1;
}

static int test_fkreplace_empty_old(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc"), *e = fkstrnew(NULL), *x = fkstrnew("X");

	CHECK(fkreplace(s, e, x, 0) == s, "expected fks returned");
	CHECK(fkreplacec(s, "", "X", 0) == s, "expected fks returned");
	CHECK(strcmp(fkcstr(s), "abc") == 0, "empty old should be a no-op, got '%s'", fkcstr(s));
	fkstrdestroy(s);
	fkstrdestroy(e);
	fkstrdestroy(x);
	return 1;
}

static int test_fkreplace_empty_fks(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);

	CHECK(fkreplacec(s, "a", "b", 0) == s, "expected fks returned");
	CHECK(fkstrlen(s) == 0 && fkstrsize(s) == 0 && fkcstr(s) == NULL, "len == 0 invariant broken");
	fkstrdestroy(s);
	return 1;
}

static int test_fkreplace_delete(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("a, b, c"), *e = fkstrnew(NULL), *sep = fkstrnew(", ");

	fkreplace(s, sep, e, 0);
	CHECK(strcmp(fkcstr(s), "abc") == 0, "expected 'abc', got '%s'", fkcstr(s));
	CHECK(fkstrlen(s) == 3, "expected len 3, got %zu", fkstrlen(s));
	fkstrdestroy(s);
	fkstrdestroy(e);
	fkstrdestroy(sep);
	return 1;
}

static int test_fkreplace_to_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("ababab");

	fkreplacec(s, "ab", "", 0);
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 0, "expected alloc 0 (len == 0 invariant), got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL (len == 0 invariant)");
	fkstrdestroy(s);
	return 1;
}

static int test_fkreplace_non_overlapping(char *errbuf, size_t errbuflen)
{
	/* Matches are found left to right and don't overlap: "aaa" has one "aa". */
	fkstring *s = fkstrnew("aaaa"), *t = fkstrnew("aaa");

	fkreplacec(s, "aa", "b", 0);
	CHECK(strcmp(fkcstr(s), "bb") == 0, "expected 'bb', got '%s'", fkcstr(s));
	fkreplacec(t, "aa", "b", 0);
	CHECK(strcmp(fkcstr(t), "ba") == 0, "expected 'ba', got '%s'", fkcstr(t));
	fkstrdestroy(s);
	fkstrdestroy(t);
	return 1;
}

static int test_fkreplace_no_rescan(char *errbuf, size_t errbuflen)
{
	/* A replacement containing old must not be matched again. */
	fkstring *s = fkstrnew("a.a");

	fkreplacec(s, "a", "aa", 0);
	CHECK(strcmp(fkcstr(s), "aa.aa") == 0, "expected 'aa.aa', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkreplace_grows_once(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);
	size_t i;

	for (i = 0; i < 100; i++)
		fkstrcatone(s, 'x');
	fkreplacec(s, "x", "abc", 0);
	CHECK(fkstrlen(s) == 300, "expected len 300, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) > fkstrlen(s), "expected alloc > len");
	CHECK(fkcstr(s)[300] == '\0', "missing terminator");
	for (i = 0; i < 300; i += 3)
		CHECK(memcmp(fkcstr(s) + i, "abc", 3) == 0, "mismatch at %zu", i);
	fkstrdestroy(s);
	return 1;
}

static int test_fkreplace_embedded_nul(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnewb("a\0b\0c", 5), *nul = fkstrnewb("\0", 1), *sep = fkstrnew("--");

	fkreplace(s, nul, sep, 0);
	CHECK(strcmp(fkcstr(s), "a--b--c") == 0, "expected 'a--b--c', got '%s'", fkcstr(s));
	fkreplace(s, sep, nul, 1);
	CHECK(fkstrlen(s) == 6 && memcmp(fkcstr(s), "a\0b--c", 7) == 0, "NUL as new: content mismatch");
	fkstrdestroy(s);
	fkstrdestroy(nul);
	fkstrdestroy(sep);
	return 1;
}

static int test_fkreplace_aliasing(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc"), *t = fkstrnew("abcabc"), *u = fkstrnew("xyz-xyz");
	fkstring *e = fkstrnew(NULL);

	fkreplace(s, s, s, 0);
	CHECK(strcmp(fkcstr(s), "abc") == 0, "old == new == fks: got '%s'", fkcstr(s));
	fkreplace(t, t, e, 0);
	CHECK(fkstrlen(t) == 0 && fkcstr(t) == NULL, "old == fks, new empty: expected empty");
	fkreplacec(u, "-", fkcstr(u), 0);	/* new points into fks's own buffer */
	CHECK(strcmp(fkcstr(u), "xyzxyz-xyzxyz") == 0, "new aliasing fks: got '%s'", fkcstr(u));
	fkstrdestroy(s);
	fkstrdestroy(t);
	fkstrdestroy(u);
	fkstrdestroy(e);
	return 1;
}

static test_case fkreplace_tests[] = {
	{ "fkreplace()/fkreplacec() with a NULL argument return NULL and leave fks alone", test_fkreplace_null_args },
	{ "fkreplacec() with max_count 0 replaces every match", test_fkreplace_all },
	{ "fkreplacec() replaces at most max_count matches, leftmost first", test_fkreplace_max_count },
	{ "fkreplacec() with no match leaves fks and its buffer untouched", test_fkreplace_no_match },
	{ "fkreplace()/fkreplacec() with an empty old are no-ops", test_fkreplace_empty_old },
	{ "fkreplacec() on an empty fks keeps the len == 0 invariant", test_fkreplace_empty_fks },
	{ "fkreplace() with an empty new deletes every match", test_fkreplace_delete },
	{ "fkreplacec() that empties fks honors the len == 0 invariant", test_fkreplace_to_empty },
	{ "fkreplacec() matches left to right without overlap", test_fkreplace_non_overlapping },
	{ "fkreplacec() does not rescan replacement text", test_fkreplace_no_rescan },
	{ "fkreplacec() growing 100 bytes to 300 produces the right content", test_fkreplace_grows_once },
	{ "fkreplace() matches and inserts embedded NULs", test_fkreplace_embedded_nul },
	{ "fkreplace()/fkreplacec() with old/new aliasing fks", test_fkreplace_aliasing },
};

test_suite fkreplace_suite = { fkreplace_tests, sizeof(fkreplace_tests) / sizeof(fkreplace_tests[0]) };
