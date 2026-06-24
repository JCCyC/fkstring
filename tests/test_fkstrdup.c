#include <string.h>
#include <fkstring.h>
#include "framework.h"

static int test_fkstrdup_null(char *errbuf, size_t errbuflen)
{
	fkstring *d = fkstrdup(NULL);

	CHECK(d == NULL, "expected NULL, got non-NULL");
	return 1;
}

static int test_fkstrdup_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);
	fkstring *d = fkstrdup(s);

	CHECK(d != NULL, "fkstrdup() of an empty fkstring returned NULL");
	CHECK(d != s, "expected a distinct fkstring object");
	CHECK(fkstrlen(d) == 0, "expected len 0, got %zu", fkstrlen(d));
	CHECK(fkstrsize(d) == 0, "expected alloc 0, got %zu", fkstrsize(d));
	CHECK(fkcstr(d) == NULL, "expected cstr NULL");
	fkstrdestroy(s);
	fkstrdestroy(d);
	return 1;
}

static int test_fkstrdup_normal(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("duplicate me");
	fkstring *d = fkstrdup(s);

	CHECK(d != s, "expected a distinct fkstring object");
	CHECK(fkcstr(d) != fkcstr(s), "expected a distinct cstr buffer");
	CHECK(fkstrlen(d) == fkstrlen(s), "expected len %zu, got %zu", fkstrlen(s), fkstrlen(d));
	CHECK(strcmp(fkcstr(d), fkcstr(s)) == 0, "content mismatch: '%s'", fkcstr(d));
	fkstrdestroy(s);
	fkstrdestroy(d);
	return 1;
}

static int test_fkstrdup_compacts_allocation(char *errbuf, size_t errbuflen)
{
	char		buf[201];
	fkstring	*s;
	fkstring	*d;
	size_t		i;

	for (i = 0; i < 200; i++)
		buf[i] = 'a' + (i % 26);
	buf[200] = '\0';

	/* len 200 gives alloc allocforlen(200) == 287. */
	s = fkstrnew(buf);
	CHECK(fkstrsize(s) == 287, "setup: expected source alloc 287, got %zu", fkstrsize(s));

	/* Truncating to 82 lands exactly on the deflate-hysteresis boundary
	 * (82*350/100 == 287 == current alloc, and the check is strictly
	 * ">"), so the source keeps its oversized 287-byte allocation. */
	fkstrtrunc(s, 82);
	CHECK(fkstrlen(s) == 82, "setup: expected len 82, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 287, "setup: expected alloc to stay 287 (hysteresis), got %zu", fkstrsize(s));

	d = fkstrdup(s);
	CHECK(fkstrlen(d) == 82, "expected len 82, got %zu", fkstrlen(d));
	CHECK(fkstrsize(d) == 118, "expected compacted alloc 118 (allocforlen(82)), got %zu", fkstrsize(d));
	CHECK(strncmp(fkcstr(d), buf, 82) == 0, "content mismatch after dup");

	fkstrdestroy(s);
	fkstrdestroy(d);
	return 1;
}

static int test_fkstrdup_is_independent(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("original");
	fkstring *d = fkstrdup(s);

	fkstrcatc(d, " modified");
	CHECK(strcmp(fkcstr(s), "original") == 0, "expected source unaffected by mutating the dup, got '%s'", fkcstr(s));
	CHECK(strcmp(fkcstr(d), "original modified") == 0, "unexpected dup content '%s'", fkcstr(d));
	fkstrdestroy(s);
	fkstrdestroy(d);
	return 1;
}

static int test_fkstrdup_embedded_nul(char *errbuf, size_t errbuflen)
{
	static const char raw[] = { 'a', '\0', 'b' };
	fkstring *s = fkstrnewb(raw, sizeof(raw));
	fkstring *d = fkstrdup(s);

	CHECK(fkstrlen(d) == sizeof(raw), "expected len %zu, got %zu", sizeof(raw), fkstrlen(d));
	CHECK(memcmp(fkcstr(d), raw, sizeof(raw)) == 0, "content mismatch around embedded NUL");
	fkstrdestroy(s);
	fkstrdestroy(d);
	return 1;
}

static test_case fkstrdup_tests[] = {
	{ "fkstrdup(NULL) returns NULL", test_fkstrdup_null },
	{ "fkstrdup() of an empty fkstring returns a distinct empty fkstring", test_fkstrdup_empty },
	{ "fkstrdup() of a normal fkstring deep-copies len and content", test_fkstrdup_normal },
	{ "fkstrdup() compacts allocation rather than copying source's alloc slack", test_fkstrdup_compacts_allocation },
	{ "fkstrdup() result is independent of mutations to the source", test_fkstrdup_is_independent },
	{ "fkstrdup() preserves embedded NUL bytes within len", test_fkstrdup_embedded_nul },
};

test_suite fkstrdup_suite = { fkstrdup_tests, sizeof(fkstrdup_tests) / sizeof(fkstrdup_tests[0]) };
