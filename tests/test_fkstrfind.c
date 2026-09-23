#include <string.h>
#include <fkstring.h>
#include "framework.h"

/* ---- fkstrfind() ---- */

static int test_fkstrfind_null(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc");
	size_t r;

	r = fkstrfind(NULL, s, 0);
	CHECK(r == FKSTR_NPOS, "fkstrfind(NULL, s, 0): expected FKSTR_NPOS, got %zu", r);
	r = fkstrfind(s, NULL, 0);
	CHECK(r == FKSTR_NPOS, "fkstrfind(s, NULL, 0): expected FKSTR_NPOS, got %zu", r);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrfind_basic(char *errbuf, size_t errbuflen)
{
	fkstring *hay = fkstrnew("the cat sat on the mat");
	fkstring *needle = fkstrnew("at");
	size_t r;

	r = fkstrfind(hay, needle, 0);
	CHECK(r == 5, "expected 5, got %zu", r);
	fkstrdestroy(hay);
	fkstrdestroy(needle);
	return 1;
}

static int test_fkstrfind_start(char *errbuf, size_t errbuflen)
{
	fkstring *hay = fkstrnew("the cat sat on the mat");
	fkstring *needle = fkstrnew("at");
	size_t r;

	r = fkstrfind(hay, needle, 5);
	CHECK(r == 5, "start at the match: expected 5, got %zu", r);
	r = fkstrfind(hay, needle, 6);
	CHECK(r == 9, "start past first match: expected 9, got %zu", r);
	r = fkstrfind(hay, needle, 21);
	CHECK(r == FKSTR_NPOS, "start inside last match: expected FKSTR_NPOS, got %zu", r);
	fkstrdestroy(hay);
	fkstrdestroy(needle);
	return 1;
}

static int test_fkstrfind_iterate(char *errbuf, size_t errbuflen)
{
	/* The documented idiom for finding every match. */
	fkstring *hay = fkstrnew("aXbXXc");
	fkstring *needle = fkstrnew("X");
	size_t expected[] = { 1, 3, 4 };
	size_t pos, n = 0;

	for (pos = fkstrfind(hay, needle, 0); pos != FKSTR_NPOS; pos = fkstrfind(hay, needle, pos + 1))
	{
		CHECK(n < 3, "too many matches");
		CHECK(pos == expected[n], "match #%zu: expected %zu, got %zu", n, expected[n], pos);
		n++;
	}
	CHECK(n == 3, "expected 3 matches, got %zu", n);
	fkstrdestroy(hay);
	fkstrdestroy(needle);
	return 1;
}

static int test_fkstrfind_not_found(char *errbuf, size_t errbuflen)
{
	fkstring *hay = fkstrnew("hello");
	fkstring *needle = fkstrnew("xyz");
	size_t r = fkstrfind(hay, needle, 0);

	CHECK(r == FKSTR_NPOS, "expected FKSTR_NPOS, got %zu", r);
	fkstrdestroy(hay);
	fkstrdestroy(needle);
	return 1;
}

static int test_fkstrfind_match_at_ends(char *errbuf, size_t errbuflen)
{
	fkstring *hay = fkstrnew("abcdef");
	fkstring *head = fkstrnew("abc");
	fkstring *tail = fkstrnew("def");
	fkstring *whole = fkstrnew("abcdef");
	size_t r;

	r = fkstrfind(hay, head, 0);
	CHECK(r == 0, "match at start: expected 0, got %zu", r);
	r = fkstrfind(hay, tail, 0);
	CHECK(r == 3, "match at end: expected 3, got %zu", r);
	r = fkstrfind(hay, whole, 0);
	CHECK(r == 0, "whole-string match: expected 0, got %zu", r);
	fkstrdestroy(hay);
	fkstrdestroy(head);
	fkstrdestroy(tail);
	fkstrdestroy(whole);
	return 1;
}

static int test_fkstrfind_needle_longer(char *errbuf, size_t errbuflen)
{
	fkstring *hay = fkstrnew("abc");
	fkstring *needle = fkstrnew("abcd");
	size_t r = fkstrfind(hay, needle, 0);

	CHECK(r == FKSTR_NPOS, "expected FKSTR_NPOS, got %zu", r);
	fkstrdestroy(hay);
	fkstrdestroy(needle);
	return 1;
}

static int test_fkstrfind_partial_overlap(char *errbuf, size_t errbuflen)
{
	/* A naive scan that skips past a failed partial match would miss this. */
	fkstring *hay = fkstrnew("aaab");
	fkstring *needle = fkstrnew("aab");
	size_t r = fkstrfind(hay, needle, 0);

	CHECK(r == 1, "expected 1, got %zu", r);
	fkstrdestroy(hay);
	fkstrdestroy(needle);
	return 1;
}

static int test_fkstrfind_empty_needle(char *errbuf, size_t errbuflen)
{
	fkstring *hay = fkstrnew("abc");
	fkstring *needle = fkstrnew(NULL);
	size_t r;

	r = fkstrfind(hay, needle, 0);
	CHECK(r == 0, "start 0: expected 0, got %zu", r);
	r = fkstrfind(hay, needle, 2);
	CHECK(r == 2, "start 2: expected 2, got %zu", r);
	r = fkstrfind(hay, needle, 3);
	CHECK(r == 3, "start == len: expected 3, got %zu", r);
	r = fkstrfind(hay, needle, 4);
	CHECK(r == FKSTR_NPOS, "start > len: expected FKSTR_NPOS, got %zu", r);
	fkstrdestroy(hay);
	fkstrdestroy(needle);
	return 1;
}

static int test_fkstrfind_empty_hay(char *errbuf, size_t errbuflen)
{
	fkstring *hay = fkstrnew(NULL);
	fkstring *needle = fkstrnew("a");
	fkstring *empty = fkstrnew(NULL);
	size_t r;

	r = fkstrfind(hay, needle, 0);
	CHECK(r == FKSTR_NPOS, "nonempty needle: expected FKSTR_NPOS, got %zu", r);
	r = fkstrfind(hay, empty, 0);
	CHECK(r == 0, "empty needle: expected 0, got %zu", r);
	fkstrdestroy(hay);
	fkstrdestroy(needle);
	fkstrdestroy(empty);
	return 1;
}

static int test_fkstrfind_start_out_of_range(char *errbuf, size_t errbuflen)
{
	fkstring *hay = fkstrnew("abc");
	fkstring *needle = fkstrnew("c");
	size_t r;

	r = fkstrfind(hay, needle, 3);
	CHECK(r == FKSTR_NPOS, "start == len: expected FKSTR_NPOS, got %zu", r);
	r = fkstrfind(hay, needle, FKSTR_NPOS);
	CHECK(r == FKSTR_NPOS, "start == FKSTR_NPOS: expected FKSTR_NPOS, got %zu", r);
	fkstrdestroy(hay);
	fkstrdestroy(needle);
	return 1;
}

static int test_fkstrfind_embedded_nul(char *errbuf, size_t errbuflen)
{
	/* strstr() would stop at the first NUL in either string. */
	fkstring *hay = fkstrnewb("ab\0cd\0ef", 8);
	fkstring *needle = fkstrnewb("d\0e", 3);
	size_t r = fkstrfind(hay, needle, 0);

	CHECK(r == 4, "expected 4, got %zu", r);
	fkstrdestroy(hay);
	fkstrdestroy(needle);
	return 1;
}

static int test_fkstrfind_self(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("same");
	size_t r = fkstrfind(s, s, 0);

	CHECK(r == 0, "expected 0, got %zu", r);
	fkstrdestroy(s);
	return 1;
}

/* ---- fkstrfindc() ---- */

static int test_fkstrfindc_null(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc");
	size_t r;

	r = fkstrfindc(NULL, "a", 0);
	CHECK(r == FKSTR_NPOS, "fkstrfindc(NULL, \"a\", 0): expected FKSTR_NPOS, got %zu", r);
	r = fkstrfindc(s, NULL, 0);
	CHECK(r == FKSTR_NPOS, "fkstrfindc(s, NULL, 0): expected FKSTR_NPOS, got %zu", r);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrfindc_basic(char *errbuf, size_t errbuflen)
{
	fkstring *hay = fkstrnew("key=value; other=thing");
	size_t r;

	r = fkstrfindc(hay, "=", 0);
	CHECK(r == 3, "expected 3, got %zu", r);
	r = fkstrfindc(hay, "=", 4);
	CHECK(r == 16, "expected 16, got %zu", r);
	r = fkstrfindc(hay, "; ", 0);
	CHECK(r == 9, "expected 9, got %zu", r);
	r = fkstrfindc(hay, "nope", 0);
	CHECK(r == FKSTR_NPOS, "expected FKSTR_NPOS, got %zu", r);
	fkstrdestroy(hay);
	return 1;
}

static int test_fkstrfindc_empty_needle(char *errbuf, size_t errbuflen)
{
	fkstring *hay = fkstrnew("abc");
	size_t r;

	r = fkstrfindc(hay, "", 1);
	CHECK(r == 1, "expected 1, got %zu", r);
	r = fkstrfindc(hay, "", 4);
	CHECK(r == FKSTR_NPOS, "start > len: expected FKSTR_NPOS, got %zu", r);
	fkstrdestroy(hay);
	return 1;
}

static int test_fkstrfindc_hay_embedded_nul(char *errbuf, size_t errbuflen)
{
	/* The needle stops at its NUL, but the haystack is searched past its own. */
	fkstring *hay = fkstrnewb("x\0needle", 8);
	size_t r = fkstrfindc(hay, "needle", 0);

	CHECK(r == 2, "expected 2, got %zu", r);
	fkstrdestroy(hay);
	return 1;
}

static test_case fkstrfind_tests[] = {
	{ "fkstrfind() returns FKSTR_NPOS for NULL arguments", test_fkstrfind_null },
	{ "fkstrfind() finds the first match", test_fkstrfind_basic },
	{ "fkstrfind() searches from start", test_fkstrfind_start },
	{ "fkstrfind() iterates over every match with pos + 1", test_fkstrfind_iterate },
	{ "fkstrfind() returns FKSTR_NPOS when there's no match", test_fkstrfind_not_found },
	{ "fkstrfind() matches at the start, end, and whole string", test_fkstrfind_match_at_ends },
	{ "fkstrfind() of a needle longer than hay is FKSTR_NPOS", test_fkstrfind_needle_longer },
	{ "fkstrfind() finds a match overlapping a failed partial match", test_fkstrfind_partial_overlap },
	{ "fkstrfind() matches an empty needle at start if start <= len", test_fkstrfind_empty_needle },
	{ "fkstrfind() on an empty hay", test_fkstrfind_empty_hay },
	{ "fkstrfind() returns FKSTR_NPOS for start out of range", test_fkstrfind_start_out_of_range },
	{ "fkstrfind() matches across embedded NULs", test_fkstrfind_embedded_nul },
	{ "fkstrfind() finds a string in itself at 0", test_fkstrfind_self },
	{ "fkstrfindc() returns FKSTR_NPOS for NULL arguments", test_fkstrfindc_null },
	{ "fkstrfindc() finds C-string needles", test_fkstrfindc_basic },
	{ "fkstrfindc() matches \"\" at start if start <= len", test_fkstrfindc_empty_needle },
	{ "fkstrfindc() searches hay past embedded NULs", test_fkstrfindc_hay_embedded_nul },
};

test_suite fkstrfind_suite = { fkstrfind_tests, sizeof(fkstrfind_tests) / sizeof(fkstrfind_tests[0]) };
