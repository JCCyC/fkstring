#include <string.h>
#include <fkstring.h>
#include "framework.h"

static int test_fkinsert_null_args(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc");

	CHECK(fkinsert(NULL, 0, s) == NULL, "fkinsert(NULL, ...) should return NULL");
	CHECK(fkinsert(s, 0, NULL) == NULL, "fkinsert(..., NULL) should return NULL");
	CHECK(fkinsertc(NULL, 0, "x") == NULL, "fkinsertc(NULL, ...) should return NULL");
	CHECK(fkinsertc(s, 0, NULL) == NULL, "fkinsertc(..., NULL) should return NULL");
	CHECK(strcmp(fkcstr(s), "abc") == 0, "fks modified: '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkinsert_pos_out_of_range(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc"), *x = fkstrnew("X");
	size_t oldalloc = fkstrsize(s);

	CHECK(fkinsert(s, 4, x) == NULL, "pos > len should return NULL");
	CHECK(fkinsertc(s, (size_t)-1, "X") == NULL, "pos == SIZE_MAX should return NULL");
	CHECK(fkstrlen(s) == 3 && fkstrsize(s) == oldalloc && strcmp(fkcstr(s), "abc") == 0,
	      "fks modified on invalid pos: '%s'", fkcstr(s));
	fkstrdestroy(s);
	fkstrdestroy(x);
	return 1;
}

static int test_fkinsert_front_middle_end(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("Demon");

	CHECK(fkinsertc(s, 3, "oti") == s, "expected fks returned for chaining");
	CHECK(strcmp(fkcstr(s), "Demotion") == 0, "middle: got '%s'", fkcstr(s));
	fkinsertc(s, 0, "[");
	CHECK(strcmp(fkcstr(s), "[Demotion") == 0, "front: got '%s'", fkcstr(s));
	fkinsertc(s, fkstrlen(s), "]");
	CHECK(strcmp(fkcstr(s), "[Demotion]") == 0, "end (pos == len): got '%s'", fkcstr(s));
	CHECK(fkstrlen(s) == 10, "expected len 10, got %zu", fkstrlen(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkinsert_inverts_fkremove(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("Demotion");
	fkstring *removed = fksubstr(s, 3, 3);

	fkremove(s, 3, 3);
	fkinsert(s, 3, removed);
	CHECK(strcmp(fkcstr(s), "Demotion") == 0, "expected 'Demotion', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	fkstrdestroy(removed);
	return 1;
}

static int test_fkinsert_into_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL), *x = fkstrnew("abc");

	CHECK(fkinsert(s, 0, x) == s, "expected fks returned");
	CHECK(fkstrlen(s) == 3 && strcmp(fkcstr(s), "abc") == 0, "got '%s'", fkcstr(s));
	CHECK(fkcstr(s) != fkcstr(x), "expected an independent copy");
	fkstrdestroy(s);
	fkstrdestroy(x);
	return 1;
}

static int test_fkinsert_empty_src(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc"), *e = fkstrnew(NULL), *empty = fkstrnew(NULL);
	char *oldbuf = fkcstr(s);

	CHECK(fkinsert(s, 1, e) == s, "expected fks returned");
	CHECK(fkinsertc(s, 3, "") == s, "expected fks returned");
	CHECK(fkcstr(s) == oldbuf && strcmp(fkcstr(s), "abc") == 0, "fks modified: '%s'", fkcstr(s));
	CHECK(fkinsert(empty, 0, e) == empty, "expected fks returned");
	CHECK(fkstrlen(empty) == 0 && fkstrsize(empty) == 0 && fkcstr(empty) == NULL,
	      "empty into empty broke the len == 0 invariant");
	fkstrdestroy(s);
	fkstrdestroy(e);
	fkstrdestroy(empty);
	return 1;
}

static int test_fkinsert_growth(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("ac");	/* alloc 16 */
	size_t i;

	for (i = 0; i < 40; i++)
		fkinsertc(s, 1, "b");
	CHECK(fkstrlen(s) == 42, "expected len 42, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) > fkstrlen(s), "expected alloc > len");
	CHECK(fkcstr(s)[0] == 'a' && fkcstr(s)[41] == 'c' && fkcstr(s)[42] == '\0', "bad ends/terminator");
	for (i = 1; i <= 40; i++)
		CHECK(fkcstr(s)[i] == 'b', "expected 'b' at %zu, got '%c'", i, fkcstr(s)[i]);
	fkstrdestroy(s);
	return 1;
}

static int test_fkinsert_embedded_nul(char *errbuf, size_t errbuflen)
{
	static const char raw[] = { 'x', '\0', 'y' };
	fkstring *s = fkstrnewb("a\0b", 3), *x = fkstrnewb(raw, sizeof(raw));

	fkinsert(s, 2, x);
	CHECK(fkstrlen(s) == 6, "expected len 6, got %zu", fkstrlen(s));
	CHECK(memcmp(fkcstr(s), "a\0x\0yb", 7) == 0, "content mismatch around embedded NULs");
	fkstrdestroy(s);
	fkstrdestroy(x);
	return 1;
}

static int test_fkinsert_self(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc"), *t = fkstrnew("0123456789abcdef0123");

	fkinsert(s, 1, s);
	CHECK(strcmp(fkcstr(s), "aabcbc") == 0, "no growth: expected 'aabcbc', got '%s'", fkcstr(s));
	fkinsert(t, 10, t);
	CHECK(strcmp(fkcstr(t), "01234567890123456789abcdef0123abcdef0123") == 0,
	      "with growth: got '%s'", fkcstr(t));
	fkstrdestroy(s);
	fkstrdestroy(t);
	return 1;
}

static int test_fkinsertc_from_own_buffer(char *errbuf, size_t errbuflen)
{
	/* The source overlaps the tail that gets shifted to make room. */
	fkstring *s = fkstrnew("abcdef");

	fkinsertc(s, 2, fkcstr(s) + 3);	/* "def" */
	CHECK(strcmp(fkcstr(s), "abdefcdef") == 0, "expected 'abdefcdef', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static test_case fkinsert_tests[] = {
	{ "fkinsert()/fkinsertc() with a NULL argument return NULL and leave fks alone", test_fkinsert_null_args },
	{ "fkinsert()/fkinsertc() with pos > len return NULL and leave fks alone", test_fkinsert_pos_out_of_range },
	{ "fkinsertc() at the front, middle, and end (pos == len) of a string", test_fkinsert_front_middle_end },
	{ "fkinsert() undoes an fkremove() of the same bytes", test_fkinsert_inverts_fkremove },
	{ "fkinsert() into an empty string deep-copies src", test_fkinsert_into_empty },
	{ "fkinsert()/fkinsertc() of an empty src leave fks untouched", test_fkinsert_empty_src },
	{ "fkinsertc() repeated 40 times in the middle grows correctly", test_fkinsert_growth },
	{ "fkinsert() preserves embedded NULs in both fks and src", test_fkinsert_embedded_nul },
	{ "fkinsert() of a string into itself, with and without growth", test_fkinsert_self },
	{ "fkinsertc() from a pointer into the tail it is about to shift", test_fkinsertc_from_own_buffer },
};

test_suite fkinsert_suite = { fkinsert_tests, sizeof(fkinsert_tests) / sizeof(fkinsert_tests[0]) };
