#include <string.h>
#include <fkstring.h>
#include "framework.h"

/* ---- fkstrchr() ---- */

static int test_fkstrchr_null(char *errbuf, size_t errbuflen)
{
	size_t r = fkstrchr(NULL, 'a', 0);

	CHECK(r == FKSTR_NPOS, "expected FKSTR_NPOS, got %zu", r);
	return 1;
}

static int test_fkstrchr_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);
	size_t r = fkstrchr(s, 'a', 0);

	CHECK(r == FKSTR_NPOS, "expected FKSTR_NPOS, got %zu", r);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrchr_basic(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("a/b/c");
	size_t r;

	r = fkstrchr(s, '/', 0);
	CHECK(r == 1, "expected 1, got %zu", r);
	r = fkstrchr(s, 'a', 0);
	CHECK(r == 0, "first byte: expected 0, got %zu", r);
	r = fkstrchr(s, 'c', 0);
	CHECK(r == 4, "last byte: expected 4, got %zu", r);
	r = fkstrchr(s, 'x', 0);
	CHECK(r == FKSTR_NPOS, "no match: expected FKSTR_NPOS, got %zu", r);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrchr_start(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("a/b/c");
	size_t r;

	r = fkstrchr(s, '/', 1);
	CHECK(r == 1, "start at the match: expected 1, got %zu", r);
	r = fkstrchr(s, '/', 2);
	CHECK(r == 3, "start past first match: expected 3, got %zu", r);
	r = fkstrchr(s, '/', 4);
	CHECK(r == FKSTR_NPOS, "start past last match: expected FKSTR_NPOS, got %zu", r);
	r = fkstrchr(s, 'c', 5);
	CHECK(r == FKSTR_NPOS, "start == len: expected FKSTR_NPOS, got %zu", r);
	r = fkstrchr(s, 'c', FKSTR_NPOS);
	CHECK(r == FKSTR_NPOS, "start == FKSTR_NPOS: expected FKSTR_NPOS, got %zu", r);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrchr_nul(char *errbuf, size_t errbuflen)
{
	/* Only embedded NULs within len match, never the terminator at cstr[len]. */
	fkstring *a = fkstrnewb("ab\0cd", 5);
	fkstring *b = fkstrnew("abcd");
	size_t r;

	r = fkstrchr(a, '\0', 0);
	CHECK(r == 2, "embedded NUL: expected 2, got %zu", r);
	r = fkstrchr(a, 'd', 0);
	CHECK(r == 4, "byte past embedded NUL: expected 4, got %zu", r);
	r = fkstrchr(b, '\0', 0);
	CHECK(r == FKSTR_NPOS, "terminator: expected FKSTR_NPOS, got %zu", r);
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkstrchr_high_byte(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnewb("a\xff" "b", 3);
	size_t r = fkstrchr(s, '\xff', 0);

	CHECK(r == 1, "expected 1, got %zu", r);
	fkstrdestroy(s);
	return 1;
}

/* ---- fkstrrchr() ---- */

static int test_fkstrrchr_null(char *errbuf, size_t errbuflen)
{
	size_t r = fkstrrchr(NULL, 'a');

	CHECK(r == FKSTR_NPOS, "expected FKSTR_NPOS, got %zu", r);
	return 1;
}

static int test_fkstrrchr_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);
	size_t r = fkstrrchr(s, 'a');

	CHECK(r == FKSTR_NPOS, "expected FKSTR_NPOS, got %zu", r);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrrchr_basic(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("/usr/local/lib");
	size_t r;

	r = fkstrrchr(s, '/');
	CHECK(r == 10, "expected 10, got %zu", r);
	r = fkstrrchr(s, 'b');
	CHECK(r == 13, "last byte: expected 13, got %zu", r);
	r = fkstrrchr(s, 'x');
	CHECK(r == FKSTR_NPOS, "no match: expected FKSTR_NPOS, got %zu", r);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrrchr_first_byte(char *errbuf, size_t errbuflen)
{
	/* Checks the loop reaches index 0 without wrapping around. */
	fkstring *s = fkstrnew("xabc");
	size_t r = fkstrrchr(s, 'x');

	CHECK(r == 0, "expected 0, got %zu", r);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrrchr_nul(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnewb("a\0b\0c", 5);
	fkstring *b = fkstrnew("abc");
	size_t r;

	r = fkstrrchr(a, '\0');
	CHECK(r == 3, "embedded NUL: expected 3, got %zu", r);
	r = fkstrrchr(b, '\0');
	CHECK(r == FKSTR_NPOS, "terminator: expected FKSTR_NPOS, got %zu", r);
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static test_case fkstrchr_tests[] = {
	{ "fkstrchr() returns FKSTR_NPOS for NULL", test_fkstrchr_null },
	{ "fkstrchr() returns FKSTR_NPOS on an empty fkstring", test_fkstrchr_empty },
	{ "fkstrchr() finds the first occurrence", test_fkstrchr_basic },
	{ "fkstrchr() searches from start", test_fkstrchr_start },
	{ "fkstrchr() finds embedded NULs but not the terminator", test_fkstrchr_nul },
	{ "fkstrchr() finds bytes >= 0x80", test_fkstrchr_high_byte },
	{ "fkstrrchr() returns FKSTR_NPOS for NULL", test_fkstrrchr_null },
	{ "fkstrrchr() returns FKSTR_NPOS on an empty fkstring", test_fkstrrchr_empty },
	{ "fkstrrchr() finds the last occurrence", test_fkstrrchr_basic },
	{ "fkstrrchr() finds a match at offset 0", test_fkstrrchr_first_byte },
	{ "fkstrrchr() finds embedded NULs but not the terminator", test_fkstrrchr_nul },
};

test_suite fkstrchr_suite = { fkstrchr_tests, sizeof(fkstrchr_tests) / sizeof(fkstrchr_tests[0]) };
