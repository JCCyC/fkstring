#include <string.h>
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

	/* _sprintftry is 48 bytes; a 100-byte result forces make_message()'s
	 * "needed more space, retry with the exact size" path. */
	for (i = 0; i < 100; i++)
		longstr[i] = 'x';
	longstr[100] = '\0';

	s = fksprintf("%s", longstr);
	CHECK(fkstrlen(s) == 100, "expected len 100, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 101, "expected alloc 101 (exact retry size), got %zu", fkstrsize(s));
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

static test_case fksprintf_tests[] = {
	{ "fksprintf() with no format specifiers returns the literal text", test_fksprintf_no_specifiers },
	{ "fksprintf() with mixed %d/%s specifiers formats correctly", test_fksprintf_mixed_specifiers },
	{ "fksprintf() output longer than _sprintftry forces the two-pass vsnprintf retry", test_fksprintf_forces_two_pass_retry },
	{ "fksprintf(\"\") honors the len==0 invariant", test_fksprintf_empty_format_honors_len_zero_invariant },
};

test_suite fksprintf_suite = { fksprintf_tests, sizeof(fksprintf_tests) / sizeof(fksprintf_tests[0]) };
