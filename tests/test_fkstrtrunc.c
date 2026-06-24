#include <string.h>
#include <fkstring.h>
#include "framework.h"

static int test_fkstrtrunc_null(char *errbuf, size_t errbuflen)
{
	fkstring *r = fkstrtrunc(NULL, 5);

	CHECK(r == NULL, "expected NULL, got non-NULL");
	return 1;
}

static int test_fkstrtrunc_already_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);
	fkstring *r = fkstrtrunc(s, 5);

	CHECK(r == s, "expected the same pointer back");
	CHECK(fkstrlen(s) == 0, "expected len to stay 0, got %zu", fkstrlen(s));
	CHECK(fkcstr(s) == NULL, "expected cstr to stay NULL");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrtrunc_to_zero(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("some content");
	fkstring *r = fkstrtrunc(s, 0);

	CHECK(r == s, "expected the same pointer back");
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 0, "expected alloc 0 per the len==0 invariant, got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL per the len==0 invariant");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrtrunc_same_len_is_noop(char *errbuf, size_t errbuflen)
{
	fkstring	*s = fkstrnew("unchanged");
	size_t		allocbefore = fkstrsize(s);

	fkstrtrunc(s, fkstrlen(s));
	CHECK(fkstrlen(s) == 9, "expected len to stay 9, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == allocbefore, "expected alloc to stay %zu, got %zu", allocbefore, fkstrsize(s));
	CHECK(strcmp(fkcstr(s), "unchanged") == 0, "expected content unchanged, got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrtrunc_larger_len_is_noop(char *errbuf, size_t errbuflen)
{
	fkstring	*s = fkstrnew("short");
	size_t		allocbefore = fkstrsize(s);

	fkstrtrunc(s, 1000);
	CHECK(fkstrlen(s) == 5, "expected len to stay 5 (truncate never grows), got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == allocbefore, "expected alloc to stay %zu, got %zu", allocbefore, fkstrsize(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrtrunc_shortens_without_shrinking_alloc(char *errbuf, size_t errbuflen)
{
	char		buf[201];
	fkstring	*s;
	size_t		i;

	for (i = 0; i < 200; i++)
		buf[i] = 'a' + (i % 26);
	buf[200] = '\0';

	/* len 200 -> alloc 287. Truncating to 82 lands exactly on the
	 * deflate-hysteresis boundary (82*350/100 == 287 == alloc, and the
	 * check is strictly ">"), so the oversized allocation is kept. */
	s = fkstrnew(buf);
	CHECK(fkstrsize(s) == 287, "setup: expected alloc 287, got %zu", fkstrsize(s));

	fkstrtrunc(s, 82);
	CHECK(fkstrlen(s) == 82, "expected len 82, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 287, "expected alloc to stay 287 (hysteresis boundary), got %zu", fkstrsize(s));
	CHECK(strncmp(fkcstr(s), buf, 82) == 0, "content mismatch");
	CHECK(fkcstr(s)[82] == '\0', "expected NUL terminator at cstr[len]");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrtrunc_shrinks_alloc_just_past_boundary(char *errbuf, size_t errbuflen)
{
	char		buf[201];
	fkstring	*s;
	size_t		i;

	for (i = 0; i < 200; i++)
		buf[i] = 'a' + (i % 26);
	buf[200] = '\0';

	/* One byte shorter than the previous case: 81*350/100 == 283 < 287,
	 * so this time the alloc > threshold check is true and the buffer
	 * shrinks to newlen+1. */
	s = fkstrnew(buf);
	fkstrtrunc(s, 81);
	CHECK(fkstrlen(s) == 81, "expected len 81, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 82, "expected alloc to shrink to 82 (newlen+1), got %zu", fkstrsize(s));
	CHECK(strncmp(fkcstr(s), buf, 81) == 0, "content mismatch");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrtrunc_shrinks_alloc_to_minalloc_floor(char *errbuf, size_t errbuflen)
{
	char		buf[201];
	fkstring	*s;
	size_t		i;

	for (i = 0; i < 200; i++)
		buf[i] = 'a' + (i % 26);
	buf[200] = '\0';

	/* newlen+1 (11) would be below _minalloc (16), so the shrink floors
	 * out at 16 instead. */
	s = fkstrnew(buf);
	fkstrtrunc(s, 10);
	CHECK(fkstrlen(s) == 10, "expected len 10, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 16, "expected alloc to floor at _minalloc (16), got %zu", fkstrsize(s));
	CHECK(strncmp(fkcstr(s), buf, 10) == 0, "content mismatch");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrtrunc_no_shrink_when_already_at_minalloc(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("ab");

	/* len 2 -> alloc 16 (the floor). Truncating to 1 satisfies
	 * alloc > newlen*350/100 (16 > 3) but NOT alloc > _minalloc
	 * (16 > 16 is false), so no realloc should happen at all. */
	CHECK(fkstrsize(s) == 16, "setup: expected alloc 16, got %zu", fkstrsize(s));

	fkstrtrunc(s, 1);
	CHECK(fkstrlen(s) == 1, "expected len 1, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 16, "expected alloc to stay 16 (already at the floor), got %zu", fkstrsize(s));
	CHECK(fkcstr(s)[0] == 'a', "expected content 'a', got '%c'", fkcstr(s)[0]);
	fkstrdestroy(s);
	return 1;
}

static test_case fkstrtrunc_tests[] = {
	{ "fkstrtrunc(NULL, ...) returns NULL", test_fkstrtrunc_null },
	{ "fkstrtrunc() on an already-empty fkstring is a no-op", test_fkstrtrunc_already_empty },
	{ "fkstrtrunc(..., 0) frees the buffer and restores the len==0 invariant", test_fkstrtrunc_to_zero },
	{ "fkstrtrunc() to the current length is a no-op", test_fkstrtrunc_same_len_is_noop },
	{ "fkstrtrunc() to a length larger than current is a no-op (never grows)", test_fkstrtrunc_larger_len_is_noop },
	{ "fkstrtrunc() shortens content but keeps the oversized alloc under the hysteresis boundary", test_fkstrtrunc_shortens_without_shrinking_alloc },
	{ "fkstrtrunc() shrinks alloc to newlen+1 just past the hysteresis boundary", test_fkstrtrunc_shrinks_alloc_just_past_boundary },
	{ "fkstrtrunc() shrinking alloc floors out at _minalloc", test_fkstrtrunc_shrinks_alloc_to_minalloc_floor },
	{ "fkstrtrunc() does not realloc when alloc is already at the _minalloc floor", test_fkstrtrunc_no_shrink_when_already_at_minalloc },
};

test_suite fkstrtrunc_suite = { fkstrtrunc_tests, sizeof(fkstrtrunc_tests) / sizeof(fkstrtrunc_tests[0]) };
