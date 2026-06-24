#include <string.h>
#include <fkstring.h>
#include "framework.h"

/* ---- fkltrim() ---- */

static int test_fkltrim_null(char *errbuf, size_t errbuflen)
{
	size_t r = fkltrim(NULL);

	CHECK(r == 0, "expected 0, got %zu", r);
	return 1;
}

static int test_fkltrim_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);
	size_t r = fkltrim(s);

	CHECK(r == 0, "expected 0, got %zu", r);
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkltrim_no_leading_space(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("hello world");
	size_t r = fkltrim(s);

	CHECK(r == 11, "expected 11, got %zu", r);
	CHECK(strcmp(fkcstr(s), "hello world") == 0, "expected unchanged, got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkltrim_leading_spaces(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("   hello");
	size_t r = fkltrim(s);

	CHECK(r == 5, "expected 5, got %zu", r);
	CHECK(fkstrlen(s) == 5, "expected len 5, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "hello") == 0, "expected 'hello', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkltrim_all_s_class_chars(char *errbuf, size_t errbuflen)
{
	/* Every character in the \s class: space, \t, \n, \r, \f, \v. */
	fkstring *s = fkstrnew(" \t\n\r\f\vdata");
	size_t r = fkltrim(s);

	CHECK(r == 4, "expected 4, got %zu", r);
	CHECK(strcmp(fkcstr(s), "data") == 0, "expected 'data', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkltrim_leaves_trailing_space_alone(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("  hello  ");
	size_t r = fkltrim(s);

	CHECK(r == 7, "expected 7, got %zu", r);
	CHECK(strcmp(fkcstr(s), "hello  ") == 0, "expected 'hello  ', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkltrim_all_whitespace_becomes_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("   \t\n  ");
	size_t r = fkltrim(s);

	CHECK(r == 0, "expected 0, got %zu", r);
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 0, "expected alloc 0 per the len==0 invariant, got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL per the len==0 invariant");
	fkstrdestroy(s);
	return 1;
}

static int test_fkltrim_leading_nul_is_not_whitespace(char *errbuf, size_t errbuflen)
{
	static const char raw[] = { '\0', ' ', 'a', 'b' };
	fkstring *s = fkstrnewb(raw, sizeof(raw));
	size_t r = fkltrim(s);

	CHECK(r == 4, "expected len unchanged at 4 (NUL is not in \\s), got %zu", r);
	CHECK(memcmp(fkcstr(s), raw, sizeof(raw)) == 0, "expected content unchanged");
	fkstrdestroy(s);
	return 1;
}

/* ---- fkrtrim() ---- */

static int test_fkrtrim_null(char *errbuf, size_t errbuflen)
{
	size_t r = fkrtrim(NULL);

	CHECK(r == 0, "expected 0, got %zu", r);
	return 1;
}

static int test_fkrtrim_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);
	size_t r = fkrtrim(s);

	CHECK(r == 0, "expected 0, got %zu", r);
	fkstrdestroy(s);
	return 1;
}

static int test_fkrtrim_no_trailing_space(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("hello world");
	size_t r = fkrtrim(s);

	CHECK(r == 11, "expected 11, got %zu", r);
	CHECK(strcmp(fkcstr(s), "hello world") == 0, "expected unchanged, got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkrtrim_trailing_spaces(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("hello   ");
	size_t r = fkrtrim(s);

	CHECK(r == 5, "expected 5, got %zu", r);
	CHECK(strcmp(fkcstr(s), "hello") == 0, "expected 'hello', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkrtrim_all_s_class_chars(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("data \t\n\r\f\v");
	size_t r = fkrtrim(s);

	CHECK(r == 4, "expected 4, got %zu", r);
	CHECK(strcmp(fkcstr(s), "data") == 0, "expected 'data', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkrtrim_leaves_leading_space_alone(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("  hello  ");
	size_t r = fkrtrim(s);

	CHECK(r == 7, "expected 7, got %zu", r);
	CHECK(strcmp(fkcstr(s), "  hello") == 0, "expected '  hello', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkrtrim_all_whitespace_becomes_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(" \t\n  ");
	size_t r = fkrtrim(s);

	CHECK(r == 0, "expected 0, got %zu", r);
	CHECK(fkstrsize(s) == 0, "expected alloc 0 per the len==0 invariant, got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL per the len==0 invariant");
	fkstrdestroy(s);
	return 1;
}

static int test_fkrtrim_trailing_nul_stops_trim(char *errbuf, size_t errbuflen)
{
	/* "abc\0   " - the trailing spaces come after an embedded NUL, which
	 * is not in \s, so trimming must stop there, not scan past it. */
	static const char raw[] = { 'a', 'b', 'c', '\0', ' ', ' ', ' ' };
	fkstring *s = fkstrnewb(raw, sizeof(raw));
	size_t r = fkrtrim(s);

	CHECK(r == 4, "expected len 4, got %zu", r);
	CHECK(memcmp(fkcstr(s), "abc\0", 4) == 0, "content mismatch after rtrim");
	fkstrdestroy(s);
	return 1;
}

/* ---- fktrim() ---- */

static int test_fktrim_null(char *errbuf, size_t errbuflen)
{
	size_t r = fktrim(NULL);

	CHECK(r == 0, "expected 0, got %zu", r);
	return 1;
}

static int test_fktrim_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);
	size_t r = fktrim(s);

	CHECK(r == 0, "expected 0, got %zu", r);
	fkstrdestroy(s);
	return 1;
}

static int test_fktrim_both_sides(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("\t\n hello \r\f");
	size_t r = fktrim(s);

	CHECK(r == 5, "expected 5, got %zu", r);
	CHECK(strcmp(fkcstr(s), "hello") == 0, "expected 'hello', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fktrim_left_only(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("   hello");
	size_t r = fktrim(s);

	CHECK(r == 5, "expected 5, got %zu", r);
	CHECK(strcmp(fkcstr(s), "hello") == 0, "expected 'hello', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fktrim_right_only(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("hello   ");
	size_t r = fktrim(s);

	CHECK(r == 5, "expected 5, got %zu", r);
	CHECK(strcmp(fkcstr(s), "hello") == 0, "expected 'hello', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fktrim_no_whitespace(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("hello");
	size_t r = fktrim(s);

	CHECK(r == 5, "expected 5, got %zu", r);
	CHECK(strcmp(fkcstr(s), "hello") == 0, "expected unchanged, got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fktrim_all_whitespace_becomes_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("  \t\n\r\f\v  ");
	size_t r = fktrim(s);

	CHECK(r == 0, "expected 0, got %zu", r);
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 0, "expected alloc 0 per the len==0 invariant, got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL per the len==0 invariant");
	fkstrdestroy(s);
	return 1;
}

static test_case fktrim_tests[] = {
	{ "fkltrim(NULL) returns 0", test_fkltrim_null },
	{ "fkltrim() on an empty fkstring is a no-op", test_fkltrim_empty },
	{ "fkltrim() with no leading whitespace is a no-op", test_fkltrim_no_leading_space },
	{ "fkltrim() removes leading spaces", test_fkltrim_leading_spaces },
	{ "fkltrim() recognizes every \\s class character at the start", test_fkltrim_all_s_class_chars },
	{ "fkltrim() leaves trailing whitespace alone", test_fkltrim_leaves_trailing_space_alone },
	{ "fkltrim() of an all-whitespace string restores the len==0 invariant", test_fkltrim_all_whitespace_becomes_empty },
	{ "fkltrim() does not treat an embedded NUL as whitespace", test_fkltrim_leading_nul_is_not_whitespace },
	{ "fkrtrim(NULL) returns 0", test_fkrtrim_null },
	{ "fkrtrim() on an empty fkstring is a no-op", test_fkrtrim_empty },
	{ "fkrtrim() with no trailing whitespace is a no-op", test_fkrtrim_no_trailing_space },
	{ "fkrtrim() removes trailing spaces", test_fkrtrim_trailing_spaces },
	{ "fkrtrim() recognizes every \\s class character at the end", test_fkrtrim_all_s_class_chars },
	{ "fkrtrim() leaves leading whitespace alone", test_fkrtrim_leaves_leading_space_alone },
	{ "fkrtrim() of an all-whitespace string restores the len==0 invariant", test_fkrtrim_all_whitespace_becomes_empty },
	{ "fkrtrim() stops at an embedded NUL rather than scanning past it", test_fkrtrim_trailing_nul_stops_trim },
	{ "fktrim(NULL) returns 0", test_fktrim_null },
	{ "fktrim() on an empty fkstring is a no-op", test_fktrim_empty },
	{ "fktrim() removes whitespace from both ends", test_fktrim_both_sides },
	{ "fktrim() with whitespace only on the left", test_fktrim_left_only },
	{ "fktrim() with whitespace only on the right", test_fktrim_right_only },
	{ "fktrim() with no whitespace is a no-op", test_fktrim_no_whitespace },
	{ "fktrim() of an all-whitespace string restores the len==0 invariant", test_fktrim_all_whitespace_becomes_empty },
};

test_suite fktrim_suite = { fktrim_tests, sizeof(fktrim_tests) / sizeof(fktrim_tests[0]) };
