#include <string.h>
#include <fkstring.h>
#include "framework.h"

static int test_fkremove_null(char *errbuf, size_t errbuflen)
{
	size_t r = fkremove(NULL, 0, 1);

	CHECK(r == 0, "expected 0, got %zu", r);
	return 1;
}

static int test_fkremove_start_at_len(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abcde");
	size_t r = fkremove(s, 5, 1);

	CHECK(r == 0, "expected 0, got %zu", r);
	CHECK(strcmp(fkcstr(s), "abcde") == 0, "expected string unchanged, got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkremove_start_past_len(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abcde");
	size_t r = fkremove(s, 100, 1);

	CHECK(r == 0, "expected 0, got %zu", r);
	CHECK(strcmp(fkcstr(s), "abcde") == 0, "expected string unchanged, got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkremove_zero_len(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abcde");
	size_t r = fkremove(s, 2, 0);

	CHECK(r == 0, "expected 0, got %zu", r);
	CHECK(strcmp(fkcstr(s), "abcde") == 0, "expected string unchanged, got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkremove_middle(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abcdefghij");
	size_t r = fkremove(s, 3, 4);

	CHECK(r == 4, "expected 4 removed, got %zu", r);
	CHECK(fkstrlen(s) == 6, "expected len 6, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "abchij") == 0, "expected 'abchij', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkremove_first_char(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abcdefghij");
	size_t r = fkremove(s, 0, 1);

	CHECK(r == 1, "expected 1 removed, got %zu", r);
	CHECK(strcmp(fkcstr(s), "bcdefghij") == 0, "expected 'bcdefghij', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkremove_to_end_no_shift_needed(char *errbuf, size_t errbuflen)
{
	/* start+len == current len: nothing trails the removed range, so
	 * the shifting memcpy must be skipped entirely. */
	fkstring *s = fkstrnew("abcdefghij");
	size_t r = fkremove(s, 7, 3);

	CHECK(r == 3, "expected 3 removed, got %zu", r);
	CHECK(strcmp(fkcstr(s), "abcdefg") == 0, "expected 'abcdefg', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkremove_len_capped_to_remaining(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abcdefghij");
	size_t r = fkremove(s, 5, 50);

	CHECK(r == 5, "expected the capped count 5 returned, got %zu", r);
	CHECK(strcmp(fkcstr(s), "abcde") == 0, "expected 'abcde', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkremove_entire_string(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abcdefghij");
	size_t r = fkremove(s, 0, 10);

	CHECK(r == 10, "expected 10 removed, got %zu", r);
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 0, "expected alloc 0 per the len==0 invariant, got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL per the len==0 invariant");
	fkstrdestroy(s);
	return 1;
}

static int test_fkremove_keeps_oversized_alloc_under_hysteresis(char *errbuf, size_t errbuflen)
{
	char		buf[201];
	fkstring	*s;
	size_t		i, r;

	for (i = 0; i < 200; i++)
		buf[i] = 'a' + (i % 26);
	buf[200] = '\0';

	/* len 200 -> alloc 287. Removing the first 118 bytes leaves len 82,
	 * landing exactly on the same deflate-hysteresis boundary exercised
	 * in the fkstrtrunc tests (82*350/100 == 287), so fkremove's internal
	 * fkstrtrunc() call should not shrink the buffer. */
	s = fkstrnew(buf);
	r = fkremove(s, 0, 118);

	CHECK(r == 118, "expected 118 removed, got %zu", r);
	CHECK(fkstrlen(s) == 82, "expected len 82, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 287, "expected alloc to stay 287 (hysteresis), got %zu", fkstrsize(s));
	CHECK(memcmp(fkcstr(s), buf + 118, 82) == 0, "content mismatch after removing the prefix");
	fkstrdestroy(s);
	return 1;
}

static test_case fkremove_tests[] = {
	{ "fkremove(NULL, ...) returns 0", test_fkremove_null },
	{ "fkremove() with start exactly at the string's length is a no-op", test_fkremove_start_at_len },
	{ "fkremove() with start past the string's length is a no-op", test_fkremove_start_past_len },
	{ "fkremove() with a zero-length removal is a no-op", test_fkremove_zero_len },
	{ "fkremove() from the middle shifts the tail left correctly", test_fkremove_middle },
	{ "fkremove() of the first character shifts everything left by one", test_fkremove_first_char },
	{ "fkremove() up to the end of the string skips the shifting memcpy", test_fkremove_to_end_no_shift_needed },
	{ "fkremove() caps len and returns the capped count when the request runs past the end", test_fkremove_len_capped_to_remaining },
	{ "fkremove() of the entire string restores the len==0 invariant", test_fkremove_entire_string },
	{ "fkremove() leaves an oversized alloc untouched under the deflate hysteresis boundary", test_fkremove_keeps_oversized_alloc_under_hysteresis },
};

test_suite fkremove_suite = { fkremove_tests, sizeof(fkremove_tests) / sizeof(fkremove_tests[0]) };
