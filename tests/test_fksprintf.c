#include <string.h>
#include <stdarg.h>
#include <fkstring.h>
#include <fkstring_internal.h>
#include "framework.h"

static int test_fksprintf_no_specifiers(char *errbuf, size_t errbuflen)
{
	fkstring *s = fksprintf("just text");

	CHECK(fkstrlen(s) == 9, "expected len 9, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "just text") == 0, "expected 'just text', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fksprintf_mixed_specifiers(char *errbuf, size_t errbuflen)
{
	fkstring *s = fksprintf("%d-%s-%d", 42, "mid", 7);

	CHECK(fkstrlen(s) == 8, "expected len 8, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "42-mid-7") == 0, "expected '42-mid-7', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fksprintf_forces_two_pass_retry(char *errbuf, size_t errbuflen)
{
	char		longstr[101];
	fkstring	*s;
	int		i;

	/* _sprintftry is 48 bytes; a 100-byte result forces fkstrcatvf()'s
	 * "needed more space, grow and retry" path. */
	for (i = 0; i < 100; i++)
		longstr[i] = 'x';
	longstr[100] = '\0';

	s = fksprintf("%s", longstr);
	CHECK(fkstrlen(s) == 100, "expected len 100, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == allocforlen(100), "expected alloc %zu (allocforlen(100)), got %zu", allocforlen(100), fkstrsize(s));
	CHECK(strcmp(fkcstr(s), longstr) == 0, "content mismatch after retry");
	fkstrdestroy(s);
	return 1;
}

static int test_fksprintf_empty_format_honors_len_zero_invariant(char *errbuf, size_t errbuflen)
{
	fkstring *s;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"
	s = fksprintf("");
#pragma GCC diagnostic pop

	CHECK(s != NULL, "fksprintf(\"\") returned NULL");
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 0, "expected alloc 0 per the len==0 invariant, got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL per the len==0 invariant, got non-NULL");
	fkstrdestroy(s);
	return 1;
}

static fkstring *wrap_fkvsprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static fkstring *wrap_fkvsprintf(const char *fmt, ...)
{
	fkstring	*ret;
	va_list		ap;

	va_start(ap, fmt);
	ret = fkvsprintf(fmt, ap);
	va_end(ap);
	return ret;
}

static int test_fkvsprintf_via_wrapper(char *errbuf, size_t errbuflen)
{
	fkstring *s = wrap_fkvsprintf("%s#%d", "item", 9);

	CHECK(s != NULL, "fkvsprintf() returned NULL");
	CHECK(fkstrlen(s) == 6, "expected len 6, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "item#9") == 0, "expected 'item#9', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkvsprintf_retry_via_wrapper(char *errbuf, size_t errbuflen)
{
	char		longstr[61];
	fkstring	*s;

	memset(longstr, 'q', 60);
	longstr[60] = '\0';
	s = wrap_fkvsprintf("<%s>%d", longstr, 5);
	CHECK(fkstrlen(s) == 63, "expected len 63, got %zu", fkstrlen(s));
	CHECK(fkcstr(s)[0] == '<' && strcmp(fkcstr(s) + 61, ">5") == 0, "content mismatch: '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fksprintf_null_fmt(char *errbuf, size_t errbuflen)
{
	const char *nullfmt = NULL;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
	CHECK(fksprintf(nullfmt) == NULL, "expected NULL for a NULL fmt");
#pragma GCC diagnostic pop
	return 1;
}

static test_case fksprintf_tests[] = {
	{ "fksprintf() with no format specifiers returns the literal text", test_fksprintf_no_specifiers },
	{ "fksprintf() with mixed %d/%s specifiers formats correctly", test_fksprintf_mixed_specifiers },
	{ "fksprintf() output longer than _sprintftry forces the two-pass vsnprintf retry", test_fksprintf_forces_two_pass_retry },
	{ "fksprintf(\"\") honors the len==0 invariant", test_fksprintf_empty_format_honors_len_zero_invariant },
	{ "fksprintf() returns NULL for a NULL fmt", test_fksprintf_null_fmt },
	{ "fkvsprintf() works from a user-written variadic wrapper", test_fkvsprintf_via_wrapper },
	{ "fkvsprintf() grow-and-retry pass still sees every argument", test_fkvsprintf_retry_via_wrapper },
};

test_suite fksprintf_suite = { fksprintf_tests, sizeof(fksprintf_tests) / sizeof(fksprintf_tests[0]) };
