#include <fkstring.h>
#include "framework.h"

/*
 * fkstrdestroy() has no observable effect besides freeing memory, so these
 * tests only confirm it doesn't crash on the shapes of fkstring it must
 * handle: NULL, an empty fkstring (cstr == NULL), and a populated one.
 */

static int test_fkstrdestroy_null(char *errbuf, size_t errbuflen)
{
	(void)errbuf;
	(void)errbuflen;
	fkstrdestroy(NULL);
	return 1;
}

static int test_fkstrdestroy_empty(char *errbuf, size_t errbuflen)
{
	(void)errbuf;
	(void)errbuflen;
	fkstrdestroy(fkstrnew(NULL));
	return 1;
}

static int test_fkstrdestroy_populated(char *errbuf, size_t errbuflen)
{
	(void)errbuf;
	(void)errbuflen;
	fkstrdestroy(fkstrnew("some content"));
	return 1;
}

static test_case fkstrdestroy_tests[] = {
	{ "fkstrdestroy(NULL) does not crash", test_fkstrdestroy_null },
	{ "fkstrdestroy() on an empty fkstring (cstr == NULL) does not crash", test_fkstrdestroy_empty },
	{ "fkstrdestroy() on a populated fkstring does not crash", test_fkstrdestroy_populated },
};

test_suite fkstrdestroy_suite = { fkstrdestroy_tests, sizeof(fkstrdestroy_tests) / sizeof(fkstrdestroy_tests[0]) };
