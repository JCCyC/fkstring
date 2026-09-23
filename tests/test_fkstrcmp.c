#include <string.h>
#include <fkstring.h>
#include "framework.h"

/* ---- fkstrcmp() ---- */

static int test_fkstrcmp_null(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);
	int r;

	r = fkstrcmp(NULL, NULL);
	CHECK(r == 0, "fkstrcmp(NULL, NULL): expected 0, got %d", r);
	r = fkstrcmp(NULL, s);
	CHECK(r < 0, "fkstrcmp(NULL, empty): expected < 0, got %d", r);
	r = fkstrcmp(s, NULL);
	CHECK(r > 0, "fkstrcmp(empty, NULL): expected > 0, got %d", r);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrcmp_both_empty(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew(NULL);
	fkstring *b = fkstrnew("");
	int r = fkstrcmp(a, b);

	CHECK(r == 0, "expected 0, got %d", r);
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkstrcmp_empty_vs_nonempty(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew(NULL);
	fkstring *b = fkstrnew("a");
	int r;

	r = fkstrcmp(a, b);
	CHECK(r < 0, "fkstrcmp(\"\", \"a\"): expected < 0, got %d", r);
	r = fkstrcmp(b, a);
	CHECK(r > 0, "fkstrcmp(\"a\", \"\"): expected > 0, got %d", r);
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkstrcmp_equal(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew("hello");
	fkstring *b = fkstrnew("hello");
	int r = fkstrcmp(a, b);

	CHECK(r == 0, "expected 0, got %d", r);
	CHECK(fkstrcmp(a, a) == 0, "expected a string to compare equal to itself");
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkstrcmp_ordering(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew("apple");
	fkstring *b = fkstrnew("apricot");
	int r;

	r = fkstrcmp(a, b);
	CHECK(r == -1, "fkstrcmp(\"apple\", \"apricot\"): expected -1, got %d", r);
	r = fkstrcmp(b, a);
	CHECK(r == 1, "fkstrcmp(\"apricot\", \"apple\"): expected 1, got %d", r);
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkstrcmp_prefix_is_shorter(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew("abc");
	fkstring *b = fkstrnew("abcd");
	int r;

	r = fkstrcmp(a, b);
	CHECK(r < 0, "fkstrcmp(\"abc\", \"abcd\"): expected < 0, got %d", r);
	r = fkstrcmp(b, a);
	CHECK(r > 0, "fkstrcmp(\"abcd\", \"abc\"): expected > 0, got %d", r);
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkstrcmp_bytes_are_unsigned(char *errbuf, size_t errbuflen)
{
	/* 0x80 must sort after 0x7f, as with memcmp(), even where char is signed. */
	fkstring *a = fkstrnewb("\x7f", 1);
	fkstring *b = fkstrnewb("\x80", 1);
	int r = fkstrcmp(a, b);

	CHECK(r < 0, "expected 0x7f < 0x80, got %d", r);
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkstrcmp_embedded_nul(char *errbuf, size_t errbuflen)
{
	/* strcmp() would stop at the NUL and call these equal. */
	fkstring *a = fkstrnewb("ab\0c", 4);
	fkstring *b = fkstrnewb("ab\0d", 4);
	fkstring *c = fkstrnewb("ab\0", 3);
	fkstring *d = fkstrnew("ab");
	int r;

	r = fkstrcmp(a, b);
	CHECK(r < 0, "expected \"ab\\0c\" < \"ab\\0d\", got %d", r);
	r = fkstrcmp(c, d);
	CHECK(r > 0, "expected \"ab\\0\" > \"ab\", got %d", r);
	fkstrdestroy(a);
	fkstrdestroy(b);
	fkstrdestroy(c);
	fkstrdestroy(d);
	return 1;
}

static int test_fkstrcmp_is_case_sensitive(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew("ABC");
	fkstring *b = fkstrnew("abc");
	int r = fkstrcmp(a, b);

	CHECK(r < 0, "expected \"ABC\" < \"abc\", got %d", r);
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

/* ---- fkstrcasecmp() ---- */

static int test_fkstrcasecmp_null(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("x");
	int r;

	r = fkstrcasecmp(NULL, NULL);
	CHECK(r == 0, "fkstrcasecmp(NULL, NULL): expected 0, got %d", r);
	r = fkstrcasecmp(NULL, s);
	CHECK(r < 0, "fkstrcasecmp(NULL, \"x\"): expected < 0, got %d", r);
	r = fkstrcasecmp(s, NULL);
	CHECK(r > 0, "fkstrcasecmp(\"x\", NULL): expected > 0, got %d", r);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrcasecmp_both_empty(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew(NULL);
	fkstring *b = fkstrnew(NULL);
	int r = fkstrcasecmp(a, b);

	CHECK(r == 0, "expected 0, got %d", r);
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkstrcasecmp_ignores_case(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew("Hello, World");
	fkstring *b = fkstrnew("hELLO, wORLD");
	int r = fkstrcasecmp(a, b);

	CHECK(r == 0, "expected 0, got %d", r);
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkstrcasecmp_ordering(char *errbuf, size_t errbuflen)
{
	/* Folded to lowercase, '_' (0x5f) < 'b' (0x62) even though 'B' (0x42) < '_'. */
	fkstring *a = fkstrnew("A_");
	fkstring *b = fkstrnew("aB");
	int r;

	r = fkstrcasecmp(a, b);
	CHECK(r == -1, "fkstrcasecmp(\"A_\", \"aB\"): expected -1, got %d", r);
	r = fkstrcasecmp(b, a);
	CHECK(r == 1, "fkstrcasecmp(\"aB\", \"A_\"): expected 1, got %d", r);
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkstrcasecmp_length_tiebreak(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew("ABC");
	fkstring *b = fkstrnew("abcd");
	int r;

	r = fkstrcasecmp(a, b);
	CHECK(r < 0, "fkstrcasecmp(\"ABC\", \"abcd\"): expected < 0, got %d", r);
	r = fkstrcasecmp(b, a);
	CHECK(r > 0, "fkstrcasecmp(\"abcd\", \"ABC\"): expected > 0, got %d", r);
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkstrcasecmp_embedded_nul(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnewb("X\0y", 3);
	fkstring *b = fkstrnewb("x\0Y", 3);
	fkstring *c = fkstrnewb("x\0Z", 3);
	int r;

	r = fkstrcasecmp(a, b);
	CHECK(r == 0, "expected \"X\\0y\" == \"x\\0Y\", got %d", r);
	r = fkstrcasecmp(a, c);
	CHECK(r < 0, "expected \"X\\0y\" < \"x\\0Z\", got %d", r);
	fkstrdestroy(a);
	fkstrdestroy(b);
	fkstrdestroy(c);
	return 1;
}

static int test_fkstrcasecmp_ascii_only(char *errbuf, size_t errbuflen)
{
	/* Latin-1 'À' (0xc0) and 'à' (0xe0) are not folded: byte-wise, ASCII only. */
	fkstring *a = fkstrnewb("\xc0", 1);
	fkstring *b = fkstrnewb("\xe0", 1);
	int r = fkstrcasecmp(a, b);

	CHECK(r < 0, "expected non-ASCII bytes to compare unfolded, got %d", r);
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

/* ---- fkstreq() ---- */

static int test_fkstreq_null(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);

	CHECK(fkstreq(NULL, NULL), "expected fkstreq(NULL, NULL) to be true");
	CHECK(!fkstreq(NULL, s), "expected fkstreq(NULL, empty) to be false");
	CHECK(!fkstreq(s, NULL), "expected fkstreq(empty, NULL) to be false");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstreq_both_empty(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew(NULL);
	fkstring *b = fkstrnew("");

	CHECK(fkstreq(a, b), "expected two empty fkstrings to be equal");
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkstreq_equal(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew("hello");
	fkstring *b = fkstrnew("hello");

	CHECK(fkstreq(a, b) == 1, "expected 1, got %d", fkstreq(a, b));
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkstreq_different_content(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew("hello");
	fkstring *b = fkstrnew("hellp");

	CHECK(fkstreq(a, b) == 0, "expected 0, got %d", fkstreq(a, b));
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkstreq_different_length(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew("hello");
	fkstring *b = fkstrnew("hello!");

	CHECK(!fkstreq(a, b), "expected strings of differing length to be unequal");
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkstreq_embedded_nul(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnewb("a\0b", 3);
	fkstring *b = fkstrnewb("a\0b", 3);
	fkstring *c = fkstrnewb("a\0c", 3);
	fkstring *d = fkstrnew("a");

	CHECK(fkstreq(a, b), "expected \"a\\0b\" == \"a\\0b\"");
	CHECK(!fkstreq(a, c), "expected \"a\\0b\" != \"a\\0c\"");
	CHECK(!fkstreq(a, d), "expected \"a\\0b\" != \"a\"");
	fkstrdestroy(a);
	fkstrdestroy(b);
	fkstrdestroy(c);
	fkstrdestroy(d);
	return 1;
}

static int test_fkstreq_is_case_sensitive(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew("abc");
	fkstring *b = fkstrnew("ABC");

	CHECK(!fkstreq(a, b), "expected \"abc\" != \"ABC\"");
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static test_case fkstrcmp_tests[] = {
	{ "fkstrcmp() sorts NULL before any fkstring, NULL == NULL", test_fkstrcmp_null },
	{ "fkstrcmp() of two empty fkstrings is 0", test_fkstrcmp_both_empty },
	{ "fkstrcmp() sorts an empty fkstring first", test_fkstrcmp_empty_vs_nonempty },
	{ "fkstrcmp() of equal strings is 0", test_fkstrcmp_equal },
	{ "fkstrcmp() orders by first differing byte, normalized to -1/1", test_fkstrcmp_ordering },
	{ "fkstrcmp() breaks ties by length", test_fkstrcmp_prefix_is_shorter },
	{ "fkstrcmp() compares bytes as unsigned", test_fkstrcmp_bytes_are_unsigned },
	{ "fkstrcmp() compares past embedded NULs", test_fkstrcmp_embedded_nul },
	{ "fkstrcmp() is case-sensitive", test_fkstrcmp_is_case_sensitive },
	{ "fkstrcasecmp() sorts NULL before any fkstring, NULL == NULL", test_fkstrcasecmp_null },
	{ "fkstrcasecmp() of two empty fkstrings is 0", test_fkstrcasecmp_both_empty },
	{ "fkstrcasecmp() ignores ASCII case", test_fkstrcasecmp_ignores_case },
	{ "fkstrcasecmp() orders by folded byte, normalized to -1/1", test_fkstrcasecmp_ordering },
	{ "fkstrcasecmp() breaks ties by length", test_fkstrcasecmp_length_tiebreak },
	{ "fkstrcasecmp() compares past embedded NULs", test_fkstrcasecmp_embedded_nul },
	{ "fkstrcasecmp() folds ASCII only, not non-ASCII bytes", test_fkstrcasecmp_ascii_only },
	{ "fkstreq() is true only for NULL/NULL among NULL arguments", test_fkstreq_null },
	{ "fkstreq() of two empty fkstrings is true", test_fkstreq_both_empty },
	{ "fkstreq() of equal strings returns 1", test_fkstreq_equal },
	{ "fkstreq() of same-length different strings returns 0", test_fkstreq_different_content },
	{ "fkstreq() of different-length strings returns 0", test_fkstreq_different_length },
	{ "fkstreq() compares past embedded NULs", test_fkstreq_embedded_nul },
	{ "fkstreq() is case-sensitive", test_fkstreq_is_case_sensitive },
};

test_suite fkstrcmp_suite = { fkstrcmp_tests, sizeof(fkstrcmp_tests) / sizeof(fkstrcmp_tests[0]) };
