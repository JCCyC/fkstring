#include <fkstring.h>
#include <fkstring_internal.h>
#include "framework.h"

/*
 * allocforlen(len) = max((len * _bumpfactor / 100) + 1, _minalloc), with
 * _bumpfactor=143 and _minalloc=16 by default. These expected values are
 * computed straight from that formula, not from re-running the code.
 */

static int test_allocforlen_zero(char *errbuf, size_t errbuflen)
{
	size_t got = allocforlen(0);
	CHECK(got == 16, "expected 16, got %zu", got);
	return 1;
}

static int test_allocforlen_below_minalloc(char *errbuf, size_t errbuflen)
{
	/* 10*143/100+1 = 15, still under _minalloc */
	size_t got = allocforlen(10);
	CHECK(got == 16, "expected 16, got %zu", got);
	return 1;
}

static int test_allocforlen_minalloc_boundary(char *errbuf, size_t errbuflen)
{
	/* 11*143/100+1 = 16 exactly: the boundary where the bump formula
	 * first reaches _minalloc on its own. */
	size_t got = allocforlen(11);
	CHECK(got == 16, "expected 16, got %zu", got);
	return 1;
}

static int test_allocforlen_above_minalloc(char *errbuf, size_t errbuflen)
{
	/* 12*143/100+1 = 18 */
	size_t got = allocforlen(12);
	CHECK(got == 18, "expected 18, got %zu", got);
	return 1;
}

static int test_allocforlen_hundred(char *errbuf, size_t errbuflen)
{
	/* 100*143/100+1 = 144 */
	size_t got = allocforlen(100);
	CHECK(got == 144, "expected 144, got %zu", got);
	return 1;
}

static int test_allocforlen_thousand(char *errbuf, size_t errbuflen)
{
	/* 1000*143/100+1 = 1431 */
	size_t got = allocforlen(1000);
	CHECK(got == 1431, "expected 1431, got %zu", got);
	return 1;
}

static test_case allocforlen_tests[] = {
	{ "allocforlen(0) returns _minalloc", test_allocforlen_zero },
	{ "allocforlen() below the _minalloc floor rounds up to _minalloc", test_allocforlen_below_minalloc },
	{ "allocforlen() lands exactly on the _minalloc boundary", test_allocforlen_minalloc_boundary },
	{ "allocforlen() just above the _minalloc boundary uses the bumped value", test_allocforlen_above_minalloc },
	{ "allocforlen(100) applies the bump factor", test_allocforlen_hundred },
	{ "allocforlen(1000) applies the bump factor at a larger scale", test_allocforlen_thousand },
};

test_suite allocforlen_suite = { allocforlen_tests, sizeof(allocforlen_tests) / sizeof(allocforlen_tests[0]) };
