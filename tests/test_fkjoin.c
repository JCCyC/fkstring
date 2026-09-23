#include <string.h>
#include <fkstring.h>
#include "framework.h"

static int test_fkjoin_null_array(char *errbuf, size_t errbuflen)
{
	fkstring *r = fkjoin(NULL, ",");

	CHECK(r == NULL, "expected NULL, got non-NULL");
	return 1;
}

static int test_fkjoin_zero_elements(char *errbuf, size_t errbuflen)
{
	fkstring *arr[] = { NULL };
	fkstring *r = fkjoin(arr, ",");

	CHECK(r != NULL, "fkjoin() of an empty array returned NULL");
	CHECK(fkstrlen(r) == 0, "expected len 0, got %zu", fkstrlen(r));
	CHECK(fkstrsize(r) == 0, "expected alloc 0 (len == 0 invariant), got %zu", fkstrsize(r));
	CHECK(fkcstr(r) == NULL, "expected cstr NULL (len == 0 invariant)");
	fkstrdestroy(r);
	return 1;
}

static int test_fkjoin_single_element(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew("hello");
	fkstring *arr[] = { a, NULL };
	fkstring *r = fkjoin(arr, ", ");

	CHECK(strcmp(fkcstr(r), "hello") == 0, "expected 'hello', got '%s'", fkcstr(r));
	CHECK(fkcstr(r) != fkcstr(a), "expected an independent copy, not aliasing the element");
	fkstrdestroy(a);
	fkstrdestroy(r);
	return 1;
}

static int test_fkjoin_multiple_elements(char *errbuf, size_t errbuflen)
{
	fkstring *arr[] = { fkstrnew("a"), fkstrnew("bb"), fkstrnew("ccc"), NULL };
	fkstring *r = fkjoin(arr, ", ");

	CHECK(fkstrlen(r) == 10, "expected len 10, got %zu", fkstrlen(r));
	CHECK(strcmp(fkcstr(r), "a, bb, ccc") == 0, "expected 'a, bb, ccc', got '%s'", fkcstr(r));
	CHECK(fkstrsize(r) > fkstrlen(r), "expected alloc > len, got alloc %zu len %zu", fkstrsize(r), fkstrlen(r));
	fkstrdestroy(arr[0]);
	fkstrdestroy(arr[1]);
	fkstrdestroy(arr[2]);
	fkstrdestroy(r);
	return 1;
}

static int test_fkjoin_empty_separator(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew("ab"), *b = fkstrnew("cd");
	fkstring *arr[] = { a, b, NULL };
	fkstring *r = fkjoin(arr, "");

	CHECK(strcmp(fkcstr(r), "abcd") == 0, "expected 'abcd', got '%s'", fkcstr(r));
	fkstrdestroy(a);
	fkstrdestroy(b);
	fkstrdestroy(r);
	return 1;
}

static int test_fkjoin_null_separator(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkstrnew("ab"), *b = fkstrnew("cd");
	fkstring *arr[] = { a, b, NULL };
	fkstring *r = fkjoin(arr, NULL);

	CHECK(r != NULL, "fkjoin() with a NULL separator returned NULL");
	CHECK(strcmp(fkcstr(r), "abcd") == 0, "expected 'abcd', got '%s'", fkcstr(r));
	fkstrdestroy(a);
	fkstrdestroy(b);
	fkstrdestroy(r);
	return 1;
}

static int test_fkjoin_empty_elements(char *errbuf, size_t errbuflen)
{
	/* Empty elements still get separators around them: {"", "a", "", ""} -> ",a,,". */
	fkstring *e1 = fkstrnew(NULL), *a = fkstrnew("a"), *e2 = fkstrnew(NULL), *e3 = fkstrnew(NULL);
	fkstring *arr[] = { e1, a, e2, e3, NULL };
	fkstring *r = fkjoin(arr, ",");

	CHECK(strcmp(fkcstr(r), ",a,,") == 0, "expected ',a,,', got '%s'", fkcstr(r));
	fkstrdestroy(e1);
	fkstrdestroy(a);
	fkstrdestroy(e2);
	fkstrdestroy(e3);
	fkstrdestroy(r);
	return 1;
}

static int test_fkjoin_all_empty_no_separator(char *errbuf, size_t errbuflen)
{
	fkstring *e1 = fkstrnew(NULL), *e2 = fkstrnew(NULL);
	fkstring *arr[] = { e1, e2, NULL };
	fkstring *r = fkjoin(arr, "");

	CHECK(fkstrlen(r) == 0, "expected len 0, got %zu", fkstrlen(r));
	CHECK(fkstrsize(r) == 0, "expected alloc 0 (len == 0 invariant), got %zu", fkstrsize(r));
	CHECK(fkcstr(r) == NULL, "expected cstr NULL (len == 0 invariant)");
	fkstrdestroy(e1);
	fkstrdestroy(e2);
	fkstrdestroy(r);
	return 1;
}

static int test_fkjoin_all_empty_with_separator(char *errbuf, size_t errbuflen)
{
	fkstring *e1 = fkstrnew(NULL), *e2 = fkstrnew(NULL), *e3 = fkstrnew(NULL);
	fkstring *arr[] = { e1, e2, e3, NULL };
	fkstring *r = fkjoin(arr, "-");

	CHECK(strcmp(fkcstr(r), "--") == 0, "expected '--', got '%s'", fkcstr(r));
	fkstrdestroy(e1);
	fkstrdestroy(e2);
	fkstrdestroy(e3);
	fkstrdestroy(r);
	return 1;
}

static int test_fkjoin_preserves_embedded_nul(char *errbuf, size_t errbuflen)
{
	static const char raw[] = { 'a', '\0', 'b' };
	fkstring *a = fkstrnewb(raw, sizeof(raw)), *b = fkstrnew("cd");
	fkstring *arr[] = { a, b, NULL };
	fkstring *r = fkjoin(arr, "+");

	CHECK(fkstrlen(r) == 6, "expected len 6, got %zu", fkstrlen(r));
	CHECK(memcmp(fkcstr(r), "a\0b+cd", 7) == 0, "content mismatch around embedded NUL");
	fkstrdestroy(a);
	fkstrdestroy(b);
	fkstrdestroy(r);
	return 1;
}

static int test_fkjoin_roundtrips_fksplit(char *errbuf, size_t errbuflen)
{
	static const char *inputs[] = { "a,b,c", ",abc", "abc,", "a,,b", ",,", "", "plain" };
	size_t i;

	for (i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++)
	{
		fkstring *s = fkstrnew(inputs[i]);
		fkstring **parts = fksplit(s, ',');
		fkstring *r = fkjoin(parts, ",");

		CHECK(fkstreq(r, s), "fkjoin(fksplit(\"%s\", ','), \",\") gave '%s'",
		      inputs[i], fkcstr(r) ? fkcstr(r) : "");
		fkstrdestroy(s);
		fkarraydestroy(parts);
		fkstrdestroy(r);
	}
	return 1;
}

static test_case fkjoin_tests[] = {
	{ "fkjoin(NULL, ...) returns NULL", test_fkjoin_null_array },
	{ "fkjoin() of an empty array returns an empty fkstring honoring the len == 0 invariant", test_fkjoin_zero_elements },
	{ "fkjoin() of a single element returns an independent copy with no separator", test_fkjoin_single_element },
	{ "fkjoin() puts the separator between elements only", test_fkjoin_multiple_elements },
	{ "fkjoin() with an empty separator concatenates", test_fkjoin_empty_separator },
	{ "fkjoin() with a NULL separator concatenates", test_fkjoin_null_separator },
	{ "fkjoin() keeps separators around empty elements", test_fkjoin_empty_elements },
	{ "fkjoin() of all-empty elements with no separator honors the len == 0 invariant", test_fkjoin_all_empty_no_separator },
	{ "fkjoin() of all-empty elements with a separator yields only separators", test_fkjoin_all_empty_with_separator },
	{ "fkjoin() preserves an embedded NUL byte within an element", test_fkjoin_preserves_embedded_nul },
	{ "fkjoin() inverts fksplit() for the same delimiter", test_fkjoin_roundtrips_fksplit },
};

test_suite fkjoin_suite = { fkjoin_tests, sizeof(fkjoin_tests) / sizeof(fkjoin_tests[0]) };
