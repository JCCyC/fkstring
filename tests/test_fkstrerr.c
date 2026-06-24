#include <string.h>
#include <fkstring.h>
#include <fkstring_internal.h>
#include "framework.h"

static int test_errmsgs_success(char *errbuf, size_t errbuflen)
{
	CHECK(strcmp(errmsgs[FKSTRERR_SUCCESS], "Success\n") == 0,
		"unexpected text: '%s'", errmsgs[FKSTRERR_SUCCESS]);
	return 1;
}

static int test_errmsgs_memalloc(char *errbuf, size_t errbuflen)
{
	CHECK(strcmp(errmsgs[FKSTRERR_MEMALLOC], "Memory alloc error\n") == 0,
		"unexpected text: '%s'", errmsgs[FKSTRERR_MEMALLOC]);
	return 1;
}

static int test_errmsgs_vsnprintf(char *errbuf, size_t errbuflen)
{
	CHECK(strcmp(errmsgs[FKSTRERR_VSNPRINTF], "Inconsistency in vsnprintf() behavior\n") == 0,
		"unexpected text: '%s'", errmsgs[FKSTRERR_VSNPRINTF]);
	return 1;
}

static test_case fkstrerr_tests[] = {
	{ "errmsgs[FKSTRERR_SUCCESS] has the expected text", test_errmsgs_success },
	{ "errmsgs[FKSTRERR_MEMALLOC] has the expected text", test_errmsgs_memalloc },
	{ "errmsgs[FKSTRERR_VSNPRINTF] has the expected text", test_errmsgs_vsnprintf },
};

test_suite fkstrerr_suite = { fkstrerr_tests, sizeof(fkstrerr_tests) / sizeof(fkstrerr_tests[0]) };
