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

/* ---- fkstartswithc() ---- */

static int test_fkstartswithc_null(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc");

	CHECK(!fkstartswithc(NULL, "a"), "expected fkstartswithc(NULL, \"a\") to be false");
	CHECK(!fkstartswithc(s, NULL), "expected fkstartswithc(s, NULL) to be false");
	CHECK(!fkstartswithc(NULL, NULL), "expected fkstartswithc(NULL, NULL) to be false");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstartswithc_basic(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("WARNING: disk full");

	CHECK(fkstartswithc(s, "WARNING: ") == 1, "expected 1, got %d", fkstartswithc(s, "WARNING: "));
	CHECK(fkstartswithc(s, "ERROR: ") == 0, "expected 0, got %d", fkstartswithc(s, "ERROR: "));
	CHECK(fkstartswithc(s, "WARNING: disk full"), "expected a string to start with itself");
	CHECK(!fkstartswithc(s, "WARNING: disk full!"), "expected a longer prefix not to match");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstartswithc_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc");
	fkstring *e = fkstrnew(NULL);

	CHECK(fkstartswithc(s, ""), "expected every string to start with \"\"");
	CHECK(fkstartswithc(e, ""), "expected \"\" to start with \"\"");
	CHECK(!fkstartswithc(e, "a"), "expected \"\" not to start with \"a\"");
	fkstrdestroy(s);
	fkstrdestroy(e);
	return 1;
}

static int test_fkstartswithc_embedded_nul(char *errbuf, size_t errbuflen)
{
	/* The C-string prefix stops at its NUL; fks may contain NULs past it. */
	fkstring *s = fkstrnewb("ab\0cd", 5);

	CHECK(fkstartswithc(s, "ab"), "expected \"ab\\0cd\" to start with \"ab\"");
	CHECK(!fkstartswithc(s, "abc"), "expected \"ab\\0cd\" not to start with \"abc\"");
	fkstrdestroy(s);
	return 1;
}

/* ---- fkendswithc() ---- */

static int test_fkendswithc_null(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc");

	CHECK(!fkendswithc(NULL, "c"), "expected fkendswithc(NULL, \"c\") to be false");
	CHECK(!fkendswithc(s, NULL), "expected fkendswithc(s, NULL) to be false");
	CHECK(!fkendswithc(NULL, NULL), "expected fkendswithc(NULL, NULL) to be false");
	fkstrdestroy(s);
	return 1;
}

static int test_fkendswithc_basic(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("file.tar.gz");

	CHECK(fkendswithc(s, ".gz") == 1, "expected 1, got %d", fkendswithc(s, ".gz"));
	CHECK(fkendswithc(s, ".tar") == 0, "expected 0, got %d", fkendswithc(s, ".tar"));
	CHECK(fkendswithc(s, "file.tar.gz"), "expected a string to end with itself");
	fkstrdestroy(s);
	return 1;
}

static int test_fkendswithc_longer_suffix(char *errbuf, size_t errbuflen)
{
	/* Guards against computing len - strlen(suffix) before the length check. */
	fkstring *s = fkstrnew("bc");

	CHECK(!fkendswithc(s, "abc"), "expected \"bc\" not to end with \"abc\"");
	fkstrdestroy(s);
	return 1;
}

static int test_fkendswithc_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc");
	fkstring *e = fkstrnew(NULL);

	CHECK(fkendswithc(s, ""), "expected every string to end with \"\"");
	CHECK(fkendswithc(e, ""), "expected \"\" to end with \"\"");
	CHECK(!fkendswithc(e, "c"), "expected \"\" not to end with \"c\"");
	fkstrdestroy(s);
	fkstrdestroy(e);
	return 1;
}

static int test_fkendswithc_embedded_nul(char *errbuf, size_t errbuflen)
{
	/* The suffix is matched against the true end (len), not the first NUL. */
	fkstring *s = fkstrnewb("ab\0cd", 5);

	CHECK(fkendswithc(s, "cd"), "expected \"ab\\0cd\" to end with \"cd\"");
	CHECK(!fkendswithc(s, "ab"), "expected \"ab\\0cd\" not to end with \"ab\"");
	fkstrdestroy(s);
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
	{ "fkstartswithc() is false for NULL arguments", test_fkstartswithc_null },
	{ "fkstartswithc() detects a C-string prefix, returning 1/0", test_fkstartswithc_basic },
	{ "fkstartswithc() treats \"\" as a prefix of everything", test_fkstartswithc_empty },
	{ "fkstartswithc() works on a string with embedded NULs", test_fkstartswithc_embedded_nul },
	{ "fkendswithc() is false for NULL arguments", test_fkendswithc_null },
	{ "fkendswithc() detects a C-string suffix, returning 1/0", test_fkendswithc_basic },
	{ "fkendswithc() is false for a suffix longer than the string", test_fkendswithc_longer_suffix },
	{ "fkendswithc() treats \"\" as a suffix of everything", test_fkendswithc_empty },
	{ "fkendswithc() matches at len, past embedded NULs", test_fkendswithc_embedded_nul },
};

test_suite fkstartswith_suite = { fkstartswith_tests, sizeof(fkstartswith_tests) / sizeof(fkstartswith_tests[0]) };
