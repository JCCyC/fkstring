#include <string.h>
#include <stdlib.h>
#include <fkstring.h>
#include "framework.h"

static size_t arraylen(fkstring **fka)
{
	size_t n = 0;

	while (fka[n])
		n++;
	return n;
}

/* ---- fksplit() ---- */

static int test_fksplit_null(char *errbuf, size_t errbuflen)
{
	fkstring **r = fksplit(NULL, ',');

	CHECK(r == NULL, "expected NULL, got non-NULL");
	return 1;
}

static int test_fksplit_empty(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(NULL);
	fkstring **parts = fksplit(s, ',');

	CHECK(parts != NULL, "fksplit() of an empty fkstring returned NULL");
	CHECK(arraylen(parts) == 1, "expected 1 part, got %zu", arraylen(parts));
	CHECK(fkstrlen(parts[0]) == 0, "expected part 0 to be empty, got len %zu", fkstrlen(parts[0]));
	fkstrdestroy(s);
	fkarraydestroy(parts);
	return 1;
}

static int test_fksplit_no_delimiter(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("hello");
	fkstring **parts = fksplit(s, ',');

	CHECK(arraylen(parts) == 1, "expected 1 part, got %zu", arraylen(parts));
	CHECK(strcmp(fkcstr(parts[0]), "hello") == 0, "expected 'hello', got '%s'", fkcstr(parts[0]));
	CHECK(fkcstr(parts[0]) != fkcstr(s), "expected an independent copy, not aliasing src");
	fkstrdestroy(s);
	fkarraydestroy(parts);
	return 1;
}

static int test_fksplit_middle(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("a,b,c");
	fkstring **parts = fksplit(s, ',');

	CHECK(arraylen(parts) == 3, "expected 3 parts, got %zu", arraylen(parts));
	CHECK(strcmp(fkcstr(parts[0]), "a") == 0, "expected 'a', got '%s'", fkcstr(parts[0]));
	CHECK(strcmp(fkcstr(parts[1]), "b") == 0, "expected 'b', got '%s'", fkcstr(parts[1]));
	CHECK(strcmp(fkcstr(parts[2]), "c") == 0, "expected 'c', got '%s'", fkcstr(parts[2]));
	CHECK(parts[3] == NULL, "expected a NULL terminator at index 3");
	fkstrdestroy(s);
	fkarraydestroy(parts);
	return 1;
}

static int test_fksplit_leading_delimiter(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(",abc");
	fkstring **parts = fksplit(s, ',');

	CHECK(arraylen(parts) == 2, "expected 2 parts, got %zu", arraylen(parts));
	CHECK(fkstrlen(parts[0]) == 0, "expected an empty leading part, got len %zu", fkstrlen(parts[0]));
	CHECK(strcmp(fkcstr(parts[1]), "abc") == 0, "expected 'abc', got '%s'", fkcstr(parts[1]));
	fkstrdestroy(s);
	fkarraydestroy(parts);
	return 1;
}

static int test_fksplit_trailing_delimiter(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("abc,");
	fkstring **parts = fksplit(s, ',');

	CHECK(arraylen(parts) == 2, "expected 2 parts, got %zu", arraylen(parts));
	CHECK(strcmp(fkcstr(parts[0]), "abc") == 0, "expected 'abc', got '%s'", fkcstr(parts[0]));
	CHECK(fkstrlen(parts[1]) == 0, "expected an empty trailing part, got len %zu", fkstrlen(parts[1]));
	fkstrdestroy(s);
	fkarraydestroy(parts);
	return 1;
}

static int test_fksplit_consecutive_delimiters(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("a,,b");
	fkstring **parts = fksplit(s, ',');

	CHECK(arraylen(parts) == 3, "expected 3 parts, got %zu", arraylen(parts));
	CHECK(strcmp(fkcstr(parts[0]), "a") == 0, "expected 'a', got '%s'", fkcstr(parts[0]));
	CHECK(fkstrlen(parts[1]) == 0, "expected an empty middle part, got len %zu", fkstrlen(parts[1]));
	CHECK(strcmp(fkcstr(parts[2]), "b") == 0, "expected 'b', got '%s'", fkcstr(parts[2]));
	fkstrdestroy(s);
	fkarraydestroy(parts);
	return 1;
}

static int test_fksplit_all_delimiters(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew(",,");
	fkstring **parts = fksplit(s, ',');

	CHECK(arraylen(parts) == 3, "expected 3 parts, got %zu", arraylen(parts));
	CHECK(fkstrlen(parts[0]) == 0, "expected part 0 empty, got len %zu", fkstrlen(parts[0]));
	CHECK(fkstrlen(parts[1]) == 0, "expected part 1 empty, got len %zu", fkstrlen(parts[1]));
	CHECK(fkstrlen(parts[2]) == 0, "expected part 2 empty, got len %zu", fkstrlen(parts[2]));
	fkstrdestroy(s);
	fkarraydestroy(parts);
	return 1;
}

static int test_fksplit_preserves_embedded_nul_in_part(char *errbuf, size_t errbuflen)
{
	/* "a\0b,cd" split on ',': first part must keep its embedded NUL. */
	static const char raw[] = { 'a', '\0', 'b', ',', 'c', 'd' };
	fkstring *s = fkstrnewb(raw, sizeof(raw));
	fkstring **parts = fksplit(s, ',');

	CHECK(arraylen(parts) == 2, "expected 2 parts, got %zu", arraylen(parts));
	CHECK(fkstrlen(parts[0]) == 3, "expected part 0 len 3, got %zu", fkstrlen(parts[0]));
	CHECK(memcmp(fkcstr(parts[0]), "a\0b", 3) == 0, "content mismatch around embedded NUL in part 0");
	CHECK(strcmp(fkcstr(parts[1]), "cd") == 0, "expected 'cd', got '%s'", fkcstr(parts[1]));
	fkstrdestroy(s);
	fkarraydestroy(parts);
	return 1;
}

static int test_fksplit_on_nul_delimiter(char *errbuf, size_t errbuflen)
{
	/* Splitting on '\0' itself: not reachable via strchr()/strlen()-based
	 * code, only correct if fksplit() walks the explicit length. */
	static const char raw[] = { 'a', 'b', '\0', 'c', 'd' };
	fkstring *s = fkstrnewb(raw, sizeof(raw));
	fkstring **parts = fksplit(s, '\0');

	CHECK(arraylen(parts) == 2, "expected 2 parts, got %zu", arraylen(parts));
	CHECK(strcmp(fkcstr(parts[0]), "ab") == 0, "expected 'ab', got '%s'", fkcstr(parts[0]));
	CHECK(strcmp(fkcstr(parts[1]), "cd") == 0, "expected 'cd', got '%s'", fkcstr(parts[1]));
	fkstrdestroy(s);
	fkarraydestroy(parts);
	return 1;
}

/* ---- fkarraydestroy() ---- */

static int test_fkarraydestroy_null(char *errbuf, size_t errbuflen)
{
	(void)errbuf;
	(void)errbuflen;
	fkarraydestroy(NULL);
	return 1;
}

static int test_fkarraydestroy_zero_elements(char *errbuf, size_t errbuflen)
{
	(void)errbuf;
	(void)errbuflen;
	fkstring **fka = malloc(sizeof(fkstring *));
	fka[0] = NULL;
	fkarraydestroy(fka);
	return 1;
}

static int test_fkarraydestroy_multiple_elements(char *errbuf, size_t errbuflen)
{
	(void)errbuf;
	(void)errbuflen;
	fkstring **fka = malloc(4 * sizeof(fkstring *));
	fka[0] = fkstrnew("one");
	fka[1] = fkstrnew(NULL); /* empty, exercises the cstr == NULL case */
	fka[2] = fkstrnew("three");
	fka[3] = NULL;
	fkarraydestroy(fka);
	return 1;
}

static test_case fksplit_tests[] = {
	{ "fksplit(NULL, ...) returns NULL", test_fksplit_null },
	{ "fksplit() of an empty fkstring returns a single empty part", test_fksplit_empty },
	{ "fksplit() with no delimiter present returns an independent single-part copy", test_fksplit_no_delimiter },
	{ "fksplit() splits on delimiters in the middle", test_fksplit_middle },
	{ "fksplit() with a leading delimiter produces an empty first part", test_fksplit_leading_delimiter },
	{ "fksplit() with a trailing delimiter produces an empty last part", test_fksplit_trailing_delimiter },
	{ "fksplit() with consecutive delimiters produces an empty part between them", test_fksplit_consecutive_delimiters },
	{ "fksplit() of a string made entirely of delimiters produces all-empty parts", test_fksplit_all_delimiters },
	{ "fksplit() preserves an embedded NUL byte within a part", test_fksplit_preserves_embedded_nul_in_part },
	{ "fksplit() can split on an embedded NUL delimiter itself", test_fksplit_on_nul_delimiter },
	{ "fkarraydestroy(NULL) does not crash", test_fkarraydestroy_null },
	{ "fkarraydestroy() on an array with zero elements does not crash", test_fkarraydestroy_zero_elements },
	{ "fkarraydestroy() frees every element including an empty one, then the array", test_fkarraydestroy_multiple_elements },
};

test_suite fksplit_suite = { fksplit_tests, sizeof(fksplit_tests) / sizeof(fksplit_tests[0]) };
