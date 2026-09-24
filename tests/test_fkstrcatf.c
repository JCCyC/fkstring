#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <wchar.h>
#include <fkstring.h>
#include <fkstring_internal.h>
#include "framework.h"

/* A user-style wrapper, the use case fkstrcatvf() exists for. */
static fkstring *wrap_fkstrcatvf(fkstring *dst, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static fkstring *wrap_fkstrcatvf(fkstring *dst, const char *fmt, ...)
{
	fkstring	*ret;
	va_list		ap;

	va_start(ap, fmt);
	ret = fkstrcatvf(dst, fmt, ap);
	va_end(ap);
	return ret;
}

/* ---- fkstrcatf() ---- */

static int test_fkstrcatf_within_slack_no_realloc(char *errbuf, size_t errbuflen)
{
	/* "hello" -> len 5, alloc 16 (slack 11); ", 42" fits, so the first
	 * vsnprintf() pass writes in place and nothing is reallocated. */
	fkstring	*s = fkstrnew("hello");
	char		*cstrbefore = fkcstr(s);

	CHECK(fkstrsize(s) == 16, "setup: expected alloc 16, got %zu", fkstrsize(s));
	fkstrcatf(s, ", %d", 42);
	CHECK(fkstrlen(s) == 9, "expected len 9, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 16, "expected alloc unchanged at 16, got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == cstrbefore, "expected cstr pointer unchanged (no realloc)");
	CHECK(strcmp(fkcstr(s), "hello, 42") == 0, "expected 'hello, 42', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrcatf_exactly_fills_slack_grows(char *errbuf, size_t errbuflen)
{
	/* Slack 11 leaves room for 10 bytes plus the NUL; 11 bytes must grow,
	 * matching fkstrcat_internal()'s "alloc - len <= srclen" rule. */
	fkstring *s = fkstrnew("hello");

	fkstrcatf(s, "%s", "0123456789");
	CHECK(fkstrsize(s) == 16, "10 bytes: expected alloc 16, got %zu", fkstrsize(s));
	fkstrtrunc(s, 5);
	fkstrcatf(s, "%s", "0123456789A");
	CHECK(fkstrlen(s) == 16, "11 bytes: expected len 16, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == allocforlen(16), "11 bytes: expected alloc %zu, got %zu", allocforlen(16), fkstrsize(s));
	CHECK(strcmp(fkcstr(s), "hello0123456789A") == 0, "content mismatch: '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrcatf_growth_uses_allocforlen(char *errbuf, size_t errbuflen)
{
	fkstring	*s = fkstrnew("abc");
	char		longstr[101];

	memset(longstr, 'x', 100);
	longstr[100] = '\0';
	fkstrcatf(s, "[%s]", longstr);
	CHECK(fkstrlen(s) == 105, "expected len 105, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == allocforlen(105), "expected alloc %zu (allocforlen(105)), got %zu", allocforlen(105), fkstrsize(s));
	CHECK(memcmp(fkcstr(s), "abc[", 4) == 0, "prefix clobbered: '%.8s'", fkcstr(s));
	CHECK(fkcstr(s)[104] == ']' && fkcstr(s)[105] == '\0', "expected ']' then NUL at the end");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrcatf_into_empty_dst(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);

	fkstrcatf(s, "%s=%d", "x", 7);
	CHECK(fkstrlen(s) == 3, "expected len 3, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == _sprintftry, "expected alloc %zu (_sprintftry), got %zu", _sprintftry, fkstrsize(s));
	CHECK(strcmp(fkcstr(s), "x=7") == 0, "expected 'x=7', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrcatf_empty_output_on_empty_dst(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);

	fkstrcatf(s, "%s", "");
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 0, "expected alloc 0 per the len==0 invariant, got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL per the len==0 invariant");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrcatf_empty_output_leaves_dst_untouched(char *errbuf, size_t errbuflen)
{
	fkstring	*s = fkstrnew("same");
	char		*cstrbefore = fkcstr(s);

	fkstrcatf(s, "%s", "");
	CHECK(fkstrlen(s) == 4, "expected len 4, got %zu", fkstrlen(s));
	CHECK(fkcstr(s) == cstrbefore, "expected cstr pointer unchanged");
	CHECK(strcmp(fkcstr(s), "same") == 0, "expected 'same', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrcatf_embedded_nuls(char *errbuf, size_t errbuflen)
{
	/* Existing embedded NULs in dst survive, and %c with 0 appends one. */
	fkstring *s = fkstrnewb("a\0b", 3);

	fkstrcatf(s, "%c%c", 0, 'z');
	CHECK(fkstrlen(s) == 5, "expected len 5, got %zu", fkstrlen(s));
	CHECK(memcmp(fkcstr(s), "a\0b\0z", 6) == 0, "expected bytes a\\0b\\0z\\0");
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrcatf_returns_dst_for_chaining(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);

	CHECK(fkstrcatf(fkstrcatf(s, "%d", 1), "-%d", 2) == s, "expected dst returned");
	CHECK(strcmp(fkcstr(s), "1-2") == 0, "expected '1-2', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrcatf_null_args(char *errbuf, size_t errbuflen)
{
	fkstring	*s = fkstrnew("keep");
	const char	*nullfmt = NULL;

	CHECK(fkstrcatf(NULL, "%d", 1) == NULL, "expected NULL for a NULL dst");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
	CHECK(fkstrcatf(s, nullfmt) == NULL, "expected NULL for a NULL fmt");
#pragma GCC diagnostic pop
	CHECK(fkstrlen(s) == 4 && strcmp(fkcstr(s), "keep") == 0, "NULL fmt modified dst");
	fkstrdestroy(s);
	return 1;
}

/* ---- fkstrcatvf() ---- */

static int test_fkstrcatvf_via_wrapper(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("log: ");

	CHECK(wrap_fkstrcatvf(s, "%s/%u", "a", 3u) == s, "expected dst returned");
	CHECK(strcmp(fkcstr(s), "log: a/3") == 0, "expected 'log: a/3', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrcatvf_second_pass_sees_all_args(char *errbuf, size_t errbuflen)
{
	/* Forces the retry through a va_list: the first pass must have run on a
	 * va_copy(), or the second would read past the arguments. */
	fkstring	*s = fkstrnew("x");
	char		longstr[81];

	memset(longstr, 'y', 80);
	longstr[80] = '\0';
	wrap_fkstrcatvf(s, "%d:%s:%d", 11, longstr, 22);
	CHECK(fkstrlen(s) == 1 + 3 + 80 + 3, "expected len 87, got %zu", fkstrlen(s));
	CHECK(memcmp(fkcstr(s), "x11:yyy", 7) == 0, "bad prefix: '%.10s'", fkcstr(s));
	CHECK(strcmp(fkcstr(s) + 81, "yyy:22") == 0, "bad suffix: '%s'", fkcstr(s) + 81);
	fkstrdestroy(s);
	return 1;
}

/* ---- vsnprintf() failure ---- */

/* In the C locale (the test run never calls setlocale()), U+0100 has no
 * multibyte form, so %ls/%lc make vsnprintf() fail with EILSEQ. */
static const wchar_t unconvertible[] = { 0x100, 0 };

static int test_fkstrcatf_vsnprintf_failure_leaves_dst_untouched(char *errbuf, size_t errbuflen)
{
	fkstring	*s = fkstrnew("keep");
	char		*cstrbefore = fkcstr(s);
	size_t		allocbefore = fkstrsize(s);

	errno = 0;
	CHECK(fkstrcatf(s, "abc%ls", unconvertible) == NULL, "expected NULL when vsnprintf() fails");
	CHECK(errno == EILSEQ, "expected errno EILSEQ, got %d", errno);
	CHECK(fkcstr(s) == cstrbefore, "expected cstr pointer unchanged");
	CHECK(fkstrsize(s) == allocbefore, "expected alloc unchanged, got %zu", fkstrsize(s));
	CHECK(fkstrlen(s) == 4, "expected len 4, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "keep") == 0, "expected 'keep' (partial output dropped), got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrcatf_vsnprintf_failure_on_empty_dst(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);

	errno = 0;
	CHECK(fkstrcatf(s, "%lc", (wint_t)unconvertible[0]) == NULL, "expected NULL when vsnprintf() fails");
	CHECK(errno == EILSEQ, "expected errno EILSEQ, got %d", errno);
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 0, "expected alloc 0 (len==0 invariant), got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL (len==0 invariant)");
	fkstrdestroy(s);
	return 1;
}

static test_case fkstrcatf_tests[] = {
	{ "fkstrcatf() within existing slack formats in place without realloc", test_fkstrcatf_within_slack_no_realloc },
	{ "fkstrcatf() grows only when the output plus NUL exceeds the slack", test_fkstrcatf_exactly_fills_slack_grows },
	{ "fkstrcatf() growth sizes the buffer via allocforlen()", test_fkstrcatf_growth_uses_allocforlen },
	{ "fkstrcatf() into an empty dst starts from a _sprintftry buffer", test_fkstrcatf_into_empty_dst },
	{ "fkstrcatf() empty output on an empty dst honors the len==0 invariant", test_fkstrcatf_empty_output_on_empty_dst },
	{ "fkstrcatf() empty output leaves a non-empty dst untouched", test_fkstrcatf_empty_output_leaves_dst_untouched },
	{ "fkstrcatf() preserves and can append embedded NULs", test_fkstrcatf_embedded_nuls },
	{ "fkstrcatf() returns dst for chaining", test_fkstrcatf_returns_dst_for_chaining },
	{ "fkstrcatf() returns NULL for a NULL dst or fmt", test_fkstrcatf_null_args },
	{ "fkstrcatvf() works from a user-written variadic wrapper", test_fkstrcatvf_via_wrapper },
	{ "fkstrcatvf() second (grow) pass still sees every argument", test_fkstrcatvf_second_pass_sees_all_args },
	{ "fkstrcatf() returns NULL with errno set and dst untouched when vsnprintf() fails", test_fkstrcatf_vsnprintf_failure_leaves_dst_untouched },
	{ "fkstrcatf() vsnprintf() failure on an empty dst honors the len==0 invariant", test_fkstrcatf_vsnprintf_failure_on_empty_dst },
};

test_suite fkstrcatf_suite = { fkstrcatf_tests, sizeof(fkstrcatf_tests) / sizeof(fkstrcatf_tests[0]) };
