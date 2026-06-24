#include <string.h>
#include <fkstring.h>
#include "framework.h"

/* ---- fkstrcat() ---- */

static int test_fkstrcat_empty_src_leaves_dst_untouched(char *errbuf, size_t errbuflen)
{
	fkstring	*dst = fkstrnew("unchanged");
	fkstring	*src = fkstrnew(NULL);
	char		*cstrbefore = fkcstr(dst);
	size_t		allocbefore = fkstrsize(dst);

	fkstrcat(dst, src);
	CHECK(fkstrlen(dst) == 9, "expected len to stay 9, got %zu", fkstrlen(dst));
	CHECK(fkcstr(dst) == cstrbefore, "expected cstr pointer unchanged (no realloc)");
	CHECK(fkstrsize(dst) == allocbefore, "expected alloc unchanged, got %zu", fkstrsize(dst));
	CHECK(strcmp(fkcstr(dst), "unchanged") == 0, "expected content unchanged, got '%s'", fkcstr(dst));
	fkstrdestroy(dst);
	fkstrdestroy(src);
	return 1;
}

static int test_fkstrcat_both_empty(char *errbuf, size_t errbuflen)
{
	fkstring *dst = fkstrnew(NULL);
	fkstring *src = fkstrnew(NULL);

	fkstrcat(dst, src);
	CHECK(fkstrlen(dst) == 0, "expected len 0, got %zu", fkstrlen(dst));
	CHECK(fkcstr(dst) == NULL, "expected cstr NULL");
	fkstrdestroy(dst);
	fkstrdestroy(src);
	return 1;
}

static int test_fkstrcat_into_empty_dst(char *errbuf, size_t errbuflen)
{
	fkstring *dst = fkstrnew(NULL);
	fkstring *src = fkstrnew("hi");

	fkstrcat(dst, src);
	CHECK(fkstrlen(dst) == 2, "expected len 2, got %zu", fkstrlen(dst));
	CHECK(fkstrsize(dst) == 16, "expected alloc 16 (allocforlen(2)), got %zu", fkstrsize(dst));
	CHECK(strcmp(fkcstr(dst), "hi") == 0, "expected 'hi', got '%s'", fkcstr(dst));
	CHECK(fkcstr(dst) != fkcstr(src), "expected dst to own a separate buffer from src");
	fkstrdestroy(dst);
	fkstrdestroy(src);
	return 1;
}

static int test_fkstrcat_no_growth_needed(char *errbuf, size_t errbuflen)
{
	/* "hello" -> len 5, alloc 16 (slack 11). Appending 5 more bytes
	 * stays within that slack, so no realloc should happen. */
	fkstring	*dst = fkstrnew("hello");
	fkstring	*src = fkstrnew("world");
	char		*cstrbefore = fkcstr(dst);

	CHECK(fkstrsize(dst) == 16, "setup: expected alloc 16, got %zu", fkstrsize(dst));
	fkstrcat(dst, src);
	CHECK(fkstrlen(dst) == 10, "expected len 10, got %zu", fkstrlen(dst));
	CHECK(fkstrsize(dst) == 16, "expected alloc to stay 16 (no growth needed), got %zu", fkstrsize(dst));
	CHECK(fkcstr(dst) == cstrbefore, "expected no realloc to have occurred");
	CHECK(strcmp(fkcstr(dst), "helloworld") == 0, "expected 'helloworld', got '%s'", fkcstr(dst));
	fkstrdestroy(dst);
	fkstrdestroy(src);
	return 1;
}

static int test_fkstrcat_triggers_growth(char *errbuf, size_t errbuflen)
{
	/* "hello" -> len 5, alloc 16 (slack 11). Appending an 11-byte src
	 * exactly exhausts the slack ((alloc-len) <= srclen), forcing a
	 * realloc to allocforlen(5+11) == allocforlen(16) == 23. */
	fkstring *dst = fkstrnew("hello");
	fkstring *src = fkstrnew("abcdefghijk");

	fkstrcat(dst, src);
	CHECK(fkstrlen(dst) == 16, "expected len 16, got %zu", fkstrlen(dst));
	CHECK(fkstrsize(dst) == 23, "expected alloc 23 (allocforlen(16)), got %zu", fkstrsize(dst));
	CHECK(strcmp(fkcstr(dst), "helloabcdefghijk") == 0, "content mismatch: '%s'", fkcstr(dst));
	fkstrdestroy(dst);
	fkstrdestroy(src);
	return 1;
}

static int test_fkstrcat_repeated_growth_stress(char *errbuf, size_t errbuflen)
{
	fkstring	*dst = fkstrnew("base");
	fkstring	*excl = fkstrnew("!");
	int		i;

	for (i = 0; i < 60; i++)
		fkstrcat(dst, excl);

	CHECK(fkstrlen(dst) == 64, "expected len 64 after 60 appends, got %zu", fkstrlen(dst));
	CHECK(strncmp(fkcstr(dst), "base", 4) == 0, "expected prefix 'base' intact");
	for (i = 4; i < 64; i++)
		CHECK(fkcstr(dst)[i] == '!', "expected '!' at offset %d, got '%c'", i, fkcstr(dst)[i]);
	CHECK(fkcstr(dst)[64] == '\0', "expected NUL terminator at cstr[len]");
	fkstrdestroy(dst);
	fkstrdestroy(excl);
	return 1;
}

static int test_fkstrcat_self_without_growth(char *errbuf, size_t errbuflen)
{
	/* "ab" -> len 2, alloc 16, plenty of slack: self-concatenation here
	 * never triggers a realloc, so it's safe regardless of how the
	 * aliasing between dst and src is handled. */
	fkstring *dst = fkstrnew("ab");

	fkstrcat(dst, dst);
	CHECK(fkstrlen(dst) == 4, "expected len 4, got %zu", fkstrlen(dst));
	CHECK(strcmp(fkcstr(dst), "abab") == 0, "expected 'abab', got '%s'", fkcstr(dst));
	fkstrdestroy(dst);
	return 1;
}

/* ---- fkstrcatc() ---- */

static int test_fkstrcatc_null_src(char *errbuf, size_t errbuflen)
{
	fkstring	*dst = fkstrnew("unchanged");
	char		*cstrbefore = fkcstr(dst);

	fkstrcatc(dst, NULL);
	CHECK(fkcstr(dst) == cstrbefore, "expected no realloc");
	CHECK(strcmp(fkcstr(dst), "unchanged") == 0, "expected content unchanged, got '%s'", fkcstr(dst));
	fkstrdestroy(dst);
	return 1;
}

static int test_fkstrcatc_empty_src(char *errbuf, size_t errbuflen)
{
	fkstring	*dst = fkstrnew("unchanged");
	char		*cstrbefore = fkcstr(dst);

	fkstrcatc(dst, "");
	CHECK(fkcstr(dst) == cstrbefore, "expected no realloc");
	CHECK(strcmp(fkcstr(dst), "unchanged") == 0, "expected content unchanged, got '%s'", fkcstr(dst));
	fkstrdestroy(dst);
	return 1;
}

static int test_fkstrcatc_into_empty_dst(char *errbuf, size_t errbuflen)
{
	fkstring *dst = fkstrnew(NULL);

	fkstrcatc(dst, "hello");
	CHECK(fkstrlen(dst) == 5, "expected len 5, got %zu", fkstrlen(dst));
	CHECK(fkstrsize(dst) == 16, "expected alloc 16, got %zu", fkstrsize(dst));
	CHECK(strcmp(fkcstr(dst), "hello") == 0, "expected 'hello', got '%s'", fkcstr(dst));
	fkstrdestroy(dst);
	return 1;
}

static int test_fkstrcatc_triggers_growth(char *errbuf, size_t errbuflen)
{
	fkstring *dst = fkstrnew("hello");

	fkstrcatc(dst, "abcdefghijk");
	CHECK(fkstrlen(dst) == 16, "expected len 16, got %zu", fkstrlen(dst));
	CHECK(fkstrsize(dst) == 23, "expected alloc 23 (allocforlen(16)), got %zu", fkstrsize(dst));
	CHECK(strcmp(fkcstr(dst), "helloabcdefghijk") == 0, "content mismatch: '%s'", fkcstr(dst));
	fkstrdestroy(dst);
	return 1;
}

static int test_fkstrcatc_special_characters(char *errbuf, size_t errbuflen)
{
	fkstring *dst = fkstrnew(NULL);

	fkstrcatc(dst, "!@#$%^&*()");
	CHECK(fkstrlen(dst) == 10, "expected len 10, got %zu", fkstrlen(dst));
	CHECK(strcmp(fkcstr(dst), "!@#$%^&*()") == 0, "content mismatch: '%s'", fkcstr(dst));
	fkstrdestroy(dst);
	return 1;
}

/* ---- fkstrcatone() ---- */

static int test_fkstrcatone_into_empty_dst(char *errbuf, size_t errbuflen)
{
	fkstring *dst = fkstrnew(NULL);

	fkstrcatone(dst, 'x');
	CHECK(fkstrlen(dst) == 1, "expected len 1, got %zu", fkstrlen(dst));
	CHECK(fkstrsize(dst) == 16, "expected alloc 16, got %zu", fkstrsize(dst));
	CHECK(fkcstr(dst)[0] == 'x', "expected 'x', got '%c'", fkcstr(dst)[0]);
	fkstrdestroy(dst);
	return 1;
}

static int test_fkstrcatone_growth_boundary(char *errbuf, size_t errbuflen)
{
	fkstring	*dst = fkstrnew("a"); /* len 1, alloc 16 */
	int		i;

	CHECK(fkstrsize(dst) == 16, "setup: expected alloc 16, got %zu", fkstrsize(dst));

	/* Grow up to len 15: slack only reaches exactly 1 at len 15, which
	 * is not yet enough to trigger a realloc (the check fires on the
	 * append that would cross it). */
	for (i = 0; i < 14; i++)
		fkstrcatone(dst, 'x');
	CHECK(fkstrlen(dst) == 15, "expected len 15, got %zu", fkstrlen(dst));
	CHECK(fkstrsize(dst) == 16, "expected alloc to still be 16 at len 15, got %zu", fkstrsize(dst));

	/* The append that takes len from 15 to 16 finds slack (alloc-len==1)
	 * <= srclen (1), so it must grow to allocforlen(16) == 23. */
	fkstrcatone(dst, 'x');
	CHECK(fkstrlen(dst) == 16, "expected len 16, got %zu", fkstrlen(dst));
	CHECK(fkstrsize(dst) == 23, "expected alloc 23 (allocforlen(16)) right after crossing, got %zu", fkstrsize(dst));

	fkstrdestroy(dst);
	return 1;
}

static int test_fkstrcatone_embedded_nul(char *errbuf, size_t errbuflen)
{
	fkstring *dst = fkstrnew("ab");

	fkstrcatone(dst, '\0');
	fkstrcatone(dst, 'c');
	CHECK(fkstrlen(dst) == 4, "expected len 4 (embedded NUL must not stop growth), got %zu", fkstrlen(dst));
	CHECK(memcmp(fkcstr(dst), "ab\0c", 4) == 0, "content mismatch around embedded NUL");
	fkstrdestroy(dst);
	return 1;
}

static test_case fkstrcat_tests[] = {
	{ "fkstrcat() with an empty src leaves dst completely untouched", test_fkstrcat_empty_src_leaves_dst_untouched },
	{ "fkstrcat() with both dst and src empty stays empty", test_fkstrcat_both_empty },
	{ "fkstrcat() into an empty dst deep-copies src into a new buffer", test_fkstrcat_into_empty_dst },
	{ "fkstrcat() within existing slack does not reallocate", test_fkstrcat_no_growth_needed },
	{ "fkstrcat() that exhausts slack triggers growth to allocforlen()", test_fkstrcat_triggers_growth },
	{ "fkstrcat() repeated 60 times accumulates content correctly across growths", test_fkstrcat_repeated_growth_stress },
	{ "fkstrcat() of a string onto itself, when no growth is needed, doubles it correctly", test_fkstrcat_self_without_growth },
	{ "fkstrcatc() with a NULL src leaves dst untouched", test_fkstrcatc_null_src },
	{ "fkstrcatc() with an empty src leaves dst untouched", test_fkstrcatc_empty_src },
	{ "fkstrcatc() into an empty dst", test_fkstrcatc_into_empty_dst },
	{ "fkstrcatc() that exhausts slack triggers growth to allocforlen()", test_fkstrcatc_triggers_growth },
	{ "fkstrcatc() appending special/punctuation characters", test_fkstrcatc_special_characters },
	{ "fkstrcatone() into an empty dst", test_fkstrcatone_into_empty_dst },
	{ "fkstrcatone() repeated across the growth boundary matches allocforlen() exactly", test_fkstrcatone_growth_boundary },
	{ "fkstrcatone() can append a literal NUL byte, preserving it within len", test_fkstrcatone_embedded_nul },
};

test_suite fkstrcat_suite = { fkstrcat_tests, sizeof(fkstrcat_tests) / sizeof(fkstrcat_tests[0]) };
