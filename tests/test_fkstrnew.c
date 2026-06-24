#include <string.h>
#include <fkstring.h>
#include "framework.h"

static int test_fkstrnew_null(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);

	CHECK(s != NULL, "fkstrnew(NULL) returned NULL");
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 0, "expected alloc 0, got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL, got non-NULL");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrnew_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("");

	CHECK(s != NULL, "fkstrnew(\"\") returned NULL");
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 0, "expected alloc 0, got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL, got non-NULL");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrnew_single_char(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("a");

	CHECK(fkstrlen(s) == 1, "expected len 1, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "a") == 0, "expected 'a', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrnew_normal(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("hello world");

	CHECK(fkstrlen(s) == 11, "expected len 11, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "hello world") == 0, "expected 'hello world', got '%s'", fkcstr(s));
	CHECK(fkcstr(s)[11] == '\0', "expected NUL terminator at cstr[len]");
	CHECK(fkstrsize(s) == 16, "expected alloc 16 (allocforlen(11)), got %zu", fkstrsize(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrnew_long_string(char *errbuf, size_t errbuflen)
{
	char		buf[5001];
	fkstring	*s;
	size_t		i;

	for (i = 0; i < 5000; i++)
		buf[i] = 'a' + (i % 26);
	buf[5000] = '\0';

	s = fkstrnew(buf);
	CHECK(fkstrlen(s) == 5000, "expected len 5000, got %zu", fkstrlen(s));
	CHECK(memcmp(fkcstr(s), buf, 5001) == 0, "content mismatch for long string");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrnewb_null_buf_zero_len(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnewb(NULL, 0);

	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 0, "expected alloc 0, got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrnewb_null_buf_nonzero_len(char *errbuf, size_t errbuflen)
{
	/* buf == NULL short-circuits regardless of len, per "if (buf && len)" */
	fkstring *s = fkstrnewb(NULL, 5);

	CHECK(fkstrlen(s) == 0, "expected len 0 when buf is NULL, got %zu", fkstrlen(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL when buf is NULL");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrnewb_zero_len_nonnull_buf(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnewb("abc", 0);

	CHECK(fkstrlen(s) == 0, "expected len 0 when len is 0, got %zu", fkstrlen(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL when len is 0");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrnewb_normal(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnewb("abcdef", 6);

	CHECK(fkstrlen(s) == 6, "expected len 6, got %zu", fkstrlen(s));
	CHECK(memcmp(fkcstr(s), "abcdef", 6) == 0, "content mismatch");
	CHECK(fkcstr(s)[6] == '\0', "expected NUL terminator at cstr[len]");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrnewb_embedded_nul(char *errbuf, size_t errbuflen)
{
	static const char raw[] = { 'a', 'b', '\0', 'c', 'd' };
	fkstring *s = fkstrnewb(raw, sizeof(raw));

	CHECK(fkstrlen(s) == 5, "expected len 5 (embedded NUL must not truncate), got %zu", fkstrlen(s));
	CHECK(memcmp(fkcstr(s), raw, sizeof(raw)) == 0, "content mismatch around embedded NUL");
	CHECK(fkcstr(s)[5] == '\0', "expected trailing NUL terminator at cstr[len]");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrnewb_binary_data(char *errbuf, size_t errbuflen)
{
	static const unsigned char raw[] = { 0x00, 0xFF, 0x41, 0x00, 0x80, 0x7F };
	fkstring *s = fkstrnewb(raw, sizeof(raw));

	CHECK(fkstrlen(s) == sizeof(raw), "expected len %zu, got %zu", sizeof(raw), fkstrlen(s));
	CHECK(memcmp(fkcstr(s), raw, sizeof(raw)) == 0, "byte content mismatch");
	fkstrdestroy(s);
	return 1;
}

static test_case fkstrnew_tests[] = {
	{ "fkstrnew(NULL) produces an empty fkstring honoring the len==0 invariant", test_fkstrnew_null },
	{ "fkstrnew(\"\") produces an empty fkstring honoring the len==0 invariant", test_fkstrnew_empty },
	{ "fkstrnew() with a single character", test_fkstrnew_single_char },
	{ "fkstrnew() with a normal string sets len, content, and alloc correctly", test_fkstrnew_normal },
	{ "fkstrnew() with a long (5000 byte) string preserves all content", test_fkstrnew_long_string },
	{ "fkstrnewb(NULL, 0) produces an empty fkstring", test_fkstrnewb_null_buf_zero_len },
	{ "fkstrnewb() with a NULL buffer ignores a nonzero len argument", test_fkstrnewb_null_buf_nonzero_len },
	{ "fkstrnewb() with len 0 ignores a non-NULL buffer argument", test_fkstrnewb_zero_len_nonnull_buf },
	{ "fkstrnewb() with a normal buffer sets len and content correctly", test_fkstrnewb_normal },
	{ "fkstrnewb() preserves embedded NUL bytes within len", test_fkstrnewb_embedded_nul },
	{ "fkstrnewb() preserves arbitrary binary byte values", test_fkstrnewb_binary_data },
};

test_suite fkstrnew_suite = { fkstrnew_tests, sizeof(fkstrnew_tests) / sizeof(fkstrnew_tests[0]) };
