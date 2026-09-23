#include <string.h>
#include <fkstring.h>
#include "framework.h"

/* ---- fkstartswith() ---- */

static int test_fkstartswith_null(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc");

	CHECK(!fkstartswith(NULL, s), "expected fkstartswith(NULL, s) to be false");
	CHECK(!fkstartswith(s, NULL), "expected fkstartswith(s, NULL) to be false");
	CHECK(!fkstartswith(NULL, NULL), "expected fkstartswith(NULL, NULL) to be false");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstartswith_basic(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("prefix-body");
	fkstring *yes = fkstrnew("prefix");
	fkstring *no = fkstrnew("body");

	CHECK(fkstartswith(s, yes) == 1, "expected 1, got %d", fkstartswith(s, yes));
	CHECK(fkstartswith(s, no) == 0, "expected 0, got %d", fkstartswith(s, no));
	CHECK(fkstartswith(s, s), "expected a string to start with itself");
	fkstrdestroy(s);
	fkstrdestroy(yes);
	fkstrdestroy(no);
	return 1;
}

static int test_fkstartswith_longer_prefix(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("ab");
	fkstring *p = fkstrnew("abc");

	CHECK(!fkstartswith(s, p), "expected \"ab\" not to start with \"abc\"");
	fkstrdestroy(s);
	fkstrdestroy(p);
	return 1;
}

static int test_fkstartswith_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc");
	fkstring *e = fkstrnew(NULL);

	CHECK(fkstartswith(s, e), "expected every string to start with \"\"");
	CHECK(fkstartswith(e, e), "expected \"\" to start with \"\"");
	CHECK(!fkstartswith(e, s), "expected \"\" not to start with \"abc\"");
	fkstrdestroy(s);
	fkstrdestroy(e);
	return 1;
}

static int test_fkstartswith_embedded_nul(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnewb("a\0b", 3);
	fkstring *yes = fkstrnewb("a\0", 2);
	fkstring *no = fkstrnewb("a\0c", 3);

	CHECK(fkstartswith(s, yes), "expected \"a\\0b\" to start with \"a\\0\"");
	CHECK(!fkstartswith(s, no), "expected \"a\\0b\" not to start with \"a\\0c\"");
	fkstrdestroy(s);
	fkstrdestroy(yes);
	fkstrdestroy(no);
	return 1;
}

/* ---- fkendswith() ---- */

static int test_fkendswith_null(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc");

	CHECK(!fkendswith(NULL, s), "expected fkendswith(NULL, s) to be false");
	CHECK(!fkendswith(s, NULL), "expected fkendswith(s, NULL) to be false");
	CHECK(!fkendswith(NULL, NULL), "expected fkendswith(NULL, NULL) to be false");
	fkstrdestroy(s);
	return 1;
}

static int test_fkendswith_basic(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("file.tar.gz");
	fkstring *yes = fkstrnew(".gz");
	fkstring *no = fkstrnew(".tar");

	CHECK(fkendswith(s, yes) == 1, "expected 1, got %d", fkendswith(s, yes));
	CHECK(fkendswith(s, no) == 0, "expected 0, got %d", fkendswith(s, no));
	CHECK(fkendswith(s, s), "expected a string to end with itself");
	fkstrdestroy(s);
	fkstrdestroy(yes);
	fkstrdestroy(no);
	return 1;
}

static int test_fkendswith_longer_suffix(char *errbuf, size_t errbuflen)
{
	/* Guards against computing len - suffix->len before the length check. */
	fkstring *s = fkstrnew("bc");
	fkstring *p = fkstrnew("abc");

	CHECK(!fkendswith(s, p), "expected \"bc\" not to end with \"abc\"");
	fkstrdestroy(s);
	fkstrdestroy(p);
	return 1;
}

static int test_fkendswith_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc");
	fkstring *e = fkstrnew(NULL);

	CHECK(fkendswith(s, e), "expected every string to end with \"\"");
	CHECK(fkendswith(e, e), "expected \"\" to end with \"\"");
	CHECK(!fkendswith(e, s), "expected \"\" not to end with \"abc\"");
	fkstrdestroy(s);
	fkstrdestroy(e);
	return 1;
}

static int test_fkendswith_embedded_nul(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnewb("a\0b", 3);
	fkstring *yes = fkstrnewb("\0b", 2);
	fkstring *no = fkstrnewb("c\0b", 3);

	CHECK(fkendswith(s, yes), "expected \"a\\0b\" to end with \"\\0b\"");
	CHECK(!fkendswith(s, no), "expected \"a\\0b\" not to end with \"c\\0b\"");
	fkstrdestroy(s);
	fkstrdestroy(yes);
	fkstrdestroy(no);
	return 1;
}

static test_case fkstartswith_tests[] = {
	{ "fkstartswith() is false for NULL arguments", test_fkstartswith_null },
	{ "fkstartswith() detects a prefix, returning 1/0", test_fkstartswith_basic },
	{ "fkstartswith() is false for a prefix longer than the string", test_fkstartswith_longer_prefix },
	{ "fkstartswith() treats \"\" as a prefix of everything", test_fkstartswith_empty },
	{ "fkstartswith() compares past embedded NULs", test_fkstartswith_embedded_nul },
	{ "fkendswith() is false for NULL arguments", test_fkendswith_null },
	{ "fkendswith() detects a suffix, returning 1/0", test_fkendswith_basic },
	{ "fkendswith() is false for a suffix longer than the string", test_fkendswith_longer_suffix },
	{ "fkendswith() treats \"\" as a suffix of everything", test_fkendswith_empty },
	{ "fkendswith() compares past embedded NULs", test_fkendswith_embedded_nul },
};

test_suite fkstartswith_suite = { fkstartswith_tests, sizeof(fkstartswith_tests) / sizeof(fkstartswith_tests[0]) };
