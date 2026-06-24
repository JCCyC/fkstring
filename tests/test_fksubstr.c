#include <string.h>
#include <fkstring.h>
#include "framework.h"

static int test_fksubstr_null(char *errbuf, size_t errbuflen)
{
	fkstring *r = fksubstr(NULL, 0, 1);

	CHECK(r == NULL, "expected NULL, got non-NULL");
	return 1;
}

static int test_fksubstr_start_at_len(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("hello");
	fkstring *r = fksubstr(s, 5, 1);

	CHECK(r != NULL, "expected a non-NULL empty fkstring, not NULL");
	CHECK(fkstrlen(r) == 0, "expected len 0, got %zu", fkstrlen(r));
	CHECK(fkcstr(r) == NULL, "expected cstr NULL");
	fkstrdestroy(s);
	fkstrdestroy(r);
	return 1;
}

static int test_fksubstr_start_past_len(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("hello");
	fkstring *r = fksubstr(s, 100, 1);

	CHECK(r != NULL, "expected a non-NULL empty fkstring, not NULL");
	CHECK(fkstrlen(r) == 0, "expected len 0, got %zu", fkstrlen(r));
	fkstrdestroy(s);
	fkstrdestroy(r);
	return 1;
}

static int test_fksubstr_zero_len(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("hello");
	fkstring *r = fksubstr(s, 2, 0);

	CHECK(r != NULL, "expected a non-NULL empty fkstring, not NULL");
	CHECK(fkstrlen(r) == 0, "expected len 0, got %zu", fkstrlen(r));
	fkstrdestroy(s);
	fkstrdestroy(r);
	return 1;
}

static int test_fksubstr_middle(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("Hello, World!");
	fkstring *r = fksubstr(s, 7, 5);

	CHECK(fkstrlen(r) == 5, "expected len 5, got %zu", fkstrlen(r));
	CHECK(fkstrsize(r) == 16, "expected alloc 16 (allocforlen(5)), got %zu", fkstrsize(r));
	CHECK(strcmp(fkcstr(r), "World") == 0, "expected 'World', got '%s'", fkcstr(r));
	fkstrdestroy(s);
	fkstrdestroy(r);
	return 1;
}

static int test_fksubstr_len_capped_to_remaining(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("Hello, World!");
	fkstring *r = fksubstr(s, 7, 100);

	CHECK(fkstrlen(r) == 6, "expected len capped to 6 (remaining chars), got %zu", fkstrlen(r));
	CHECK(strcmp(fkcstr(r), "World!") == 0, "expected 'World!', got '%s'", fkcstr(r));
	fkstrdestroy(s);
	fkstrdestroy(r);
	return 1;
}

static int test_fksubstr_full_copy_is_independent(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("Hello, World!");
	fkstring *r = fksubstr(s, 0, fkstrlen(s));

	CHECK(fkstrlen(r) == fkstrlen(s), "expected len %zu, got %zu", fkstrlen(s), fkstrlen(r));
	CHECK(fkcstr(r) != fkcstr(s), "expected a distinct buffer from the source");
	CHECK(strcmp(fkcstr(r), fkcstr(s)) == 0, "content mismatch: '%s'", fkcstr(r));
	fkstrdestroy(s);
	fkstrdestroy(r);
	return 1;
}

static int test_fksubstr_last_char(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("Hello, World!");
	fkstring *r = fksubstr(s, fkstrlen(s) - 1, 1);

	CHECK(fkstrlen(r) == 1, "expected len 1, got %zu", fkstrlen(r));
	CHECK(fkcstr(r)[0] == '!', "expected '!', got '%c'", fkcstr(r)[0]);
	fkstrdestroy(s);
	fkstrdestroy(r);
	return 1;
}

static int test_fksubstr_embedded_nul(char *errbuf, size_t errbuflen)
{
	static const char raw[] = { 'a', 'b', '\0', 'c', 'd', 'e' };
	fkstring *s = fkstrnewb(raw, sizeof(raw));
	fkstring *r = fksubstr(s, 1, 4);

	CHECK(fkstrlen(r) == 4, "expected len 4 (embedded NUL must not truncate), got %zu", fkstrlen(r));
	CHECK(memcmp(fkcstr(r), "b\0cd", 4) == 0, "content mismatch around embedded NUL");
	fkstrdestroy(s);
	fkstrdestroy(r);
	return 1;
}

static test_case fksubstr_tests[] = {
	{ "fksubstr(NULL, ...) returns NULL", test_fksubstr_null },
	{ "fksubstr() with start exactly at the string's length returns an empty fkstring", test_fksubstr_start_at_len },
	{ "fksubstr() with start far beyond the string's length returns an empty fkstring", test_fksubstr_start_past_len },
	{ "fksubstr() with a zero-length request returns an empty fkstring", test_fksubstr_zero_len },
	{ "fksubstr() extracts a middle substring with correct content and alloc", test_fksubstr_middle },
	{ "fksubstr() caps len when the request runs past the end of the string", test_fksubstr_len_capped_to_remaining },
	{ "fksubstr() of the entire string returns an independent copy", test_fksubstr_full_copy_is_independent },
	{ "fksubstr() can extract a single trailing character", test_fksubstr_last_char },
	{ "fksubstr() preserves embedded NUL bytes within the extracted range", test_fksubstr_embedded_nul },
};

test_suite fksubstr_suite = { fksubstr_tests, sizeof(fksubstr_tests) / sizeof(fksubstr_tests[0]) };
