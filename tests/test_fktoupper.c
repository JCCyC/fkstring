#include <string.h>
#include <fkstring.h>
#include "framework.h"

/* ---- fktoupper() ---- */

static int test_fktoupper_null(char *errbuf, size_t errbuflen)
{
	fkstring *r = fktoupper(NULL);

	CHECK(r == NULL, "expected NULL, got %p", (void *)r);
	return 1;
}

static int test_fktoupper_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);
	fkstring *r = fktoupper(s);

	CHECK(r == s, "expected fks back for chaining");
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 0, "expected alloc 0 per the len==0 invariant, got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL per the len==0 invariant");
	fkstrdestroy(s);
	return 1;
}

static int test_fktoupper_mixed(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("Hello, World 42!");
	fkstring *r = fktoupper(s);

	CHECK(r == s, "expected fks back for chaining");
	CHECK(fkstrlen(s) == 16, "expected len 16, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "HELLO, WORLD 42!") == 0, "expected 'HELLO, WORLD 42!', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fktoupper_ascii_boundaries(char *errbuf, size_t errbuflen)
{
	/* The bytes on either side of 'a'..'z' ('`' and '{') must not move. */
	fkstring *s = fkstrnew("`az{@AZ[");

	fktoupper(s);
	CHECK(strcmp(fkcstr(s), "`AZ{@AZ[") == 0, "expected '`AZ{@AZ[', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fktoupper_past_embedded_nul(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnewb("ab\0cd", 5);

	fktoupper(s);
	CHECK(fkstrlen(s) == 5, "expected len 5, got %zu", fkstrlen(s));
	CHECK(memcmp(fkcstr(s), "AB\0CD", 6) == 0, "expected \"AB\\0CD\" plus terminator");
	fkstrdestroy(s);
	return 1;
}

static int test_fktoupper_leaves_non_ascii_alone(char *errbuf, size_t errbuflen)
{
	/* UTF-8 "é" (0xC3 0xA9) and Latin-1 "é" (0xE9): byte-wise, not locale-aware. */
	fkstring *s = fkstrnew("caf\xc3\xa9 \xe9");

	fktoupper(s);
	CHECK(strcmp(fkcstr(s), "CAF\xc3\xa9 \xe9") == 0, "expected non-ASCII bytes unchanged");
	fkstrdestroy(s);
	return 1;
}

static int test_fktoupper_keeps_alloc(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc");
	size_t alloc;

	fkslack(s, 100);
	alloc = fkstrsize(s);
	fktoupper(s);
	CHECK(fkstrsize(s) == alloc, "expected alloc %zu, got %zu", alloc, fkstrsize(s));
	fkstrdestroy(s);
	return 1;
}

/* ---- fktolower() ---- */

static int test_fktolower_null(char *errbuf, size_t errbuflen)
{
	fkstring *r = fktolower(NULL);

	CHECK(r == NULL, "expected NULL, got %p", (void *)r);
	return 1;
}

static int test_fktolower_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);
	fkstring *r = fktolower(s);

	CHECK(r == s, "expected fks back for chaining");
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 0, "expected alloc 0 per the len==0 invariant, got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL per the len==0 invariant");
	fkstrdestroy(s);
	return 1;
}

static int test_fktolower_mixed(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("Hello, World 42!");
	fkstring *r = fktolower(s);

	CHECK(r == s, "expected fks back for chaining");
	CHECK(strcmp(fkcstr(s), "hello, world 42!") == 0, "expected 'hello, world 42!', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fktolower_ascii_boundaries(char *errbuf, size_t errbuflen)
{
	/* The bytes on either side of 'A'..'Z' ('@' and '[') must not move. */
	fkstring *s = fkstrnew("@AZ[`az{");

	fktolower(s);
	CHECK(strcmp(fkcstr(s), "@az[`az{") == 0, "expected '@az[`az{', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fktolower_past_embedded_nul(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnewb("AB\0CD", 5);

	fktolower(s);
	CHECK(fkstrlen(s) == 5, "expected len 5, got %zu", fkstrlen(s));
	CHECK(memcmp(fkcstr(s), "ab\0cd", 6) == 0, "expected \"ab\\0cd\" plus terminator");
	fkstrdestroy(s);
	return 1;
}

static int test_fktolower_leaves_non_ascii_alone(char *errbuf, size_t errbuflen)
{
	/* UTF-8 "É" (0xC3 0x89) and Latin-1 "É" (0xC9). */
	fkstring *s = fkstrnew("CAF\xc3\x89 \xc9");

	fktolower(s);
	CHECK(strcmp(fkcstr(s), "caf\xc3\x89 \xc9") == 0, "expected non-ASCII bytes unchanged");
	fkstrdestroy(s);
	return 1;
}

static int test_fktolower_agrees_with_fkstrcasecmp(char *errbuf, size_t errbuflen)
{
	/* Both use the same ASCII-only folding, so lowercased copies of two
	 * strings are equal exactly when fkstrcasecmp() says they are. */
	fkstring *a = fkstrnew("MiXeD \xc9 Case");
	fkstring *b = fkstrnew("mIxEd \xc9 cASE");

	CHECK(fkstrcasecmp(a, b) == 0, "expected fkstrcasecmp() == 0");
	fktolower(a);
	fktolower(b);
	CHECK(fkstreq(a, b), "expected lowercased strings to be equal");
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fktolower_chains(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("ABC");

	fkstrcatc(fktolower(s), "DEF");
	CHECK(strcmp(fkcstr(s), "abcDEF") == 0, "expected 'abcDEF', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static test_case fktoupper_tests[] = {
	{ "fktoupper(NULL) returns NULL", test_fktoupper_null },
	{ "fktoupper() on an empty fkstring is a no-op", test_fktoupper_empty },
	{ "fktoupper() uppercases letters and leaves the rest alone", test_fktoupper_mixed },
	{ "fktoupper() only touches 'a'..'z'", test_fktoupper_ascii_boundaries },
	{ "fktoupper() converts past an embedded NUL", test_fktoupper_past_embedded_nul },
	{ "fktoupper() leaves non-ASCII bytes alone", test_fktoupper_leaves_non_ascii_alone },
	{ "fktoupper() does not reallocate", test_fktoupper_keeps_alloc },
	{ "fktolower(NULL) returns NULL", test_fktolower_null },
	{ "fktolower() on an empty fkstring is a no-op", test_fktolower_empty },
	{ "fktolower() lowercases letters and leaves the rest alone", test_fktolower_mixed },
	{ "fktolower() only touches 'A'..'Z'", test_fktolower_ascii_boundaries },
	{ "fktolower() converts past an embedded NUL", test_fktolower_past_embedded_nul },
	{ "fktolower() leaves non-ASCII bytes alone", test_fktolower_leaves_non_ascii_alone },
	{ "fktolower() folds exactly like fkstrcasecmp()", test_fktolower_agrees_with_fkstrcasecmp },
	{ "fktolower() returns fks for chaining", test_fktolower_chains },
};

test_suite fktoupper_suite = { fktoupper_tests, sizeof(fktoupper_tests) / sizeof(fktoupper_tests[0]) };
