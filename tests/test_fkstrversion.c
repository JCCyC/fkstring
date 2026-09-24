#include <stdio.h>
#include <string.h>
#include <fkstring.h>
#include "framework.h"

static int test_fkstrversion_matches_header(char *errbuf, size_t errbuflen)
{
	const char	*v = fkstrversion();

	CHECK(v != NULL, "fkstrversion() returned NULL");
	CHECK(strcmp(v, FKSTRING_VERSION) == 0,
		"library is '%s', header is '%s'", v, FKSTRING_VERSION);
	return 1;
}

static int test_version_string_matches_components(char *errbuf, size_t errbuflen)
{
	char	buf[64];

	snprintf(buf, sizeof(buf), "%d.%d.%d", FKSTRING_VERSION_MAJOR,
		FKSTRING_VERSION_MINOR, FKSTRING_VERSION_PATCH);
	CHECK(strcmp(buf, FKSTRING_VERSION) == 0,
		"components give '%s', FKSTRING_VERSION is '%s'", buf, FKSTRING_VERSION);
	return 1;
}

static int test_version_number_matches_components(char *errbuf, size_t errbuflen)
{
	CHECK(FKSTRING_VERSION_MINOR < 100 && FKSTRING_VERSION_PATCH < 100,
		"minor/patch must stay below 100 for FKSTRING_VERSION_NUMBER");
	CHECK(FKSTRING_VERSION_NUMBER / 10000 == FKSTRING_VERSION_MAJOR &&
		FKSTRING_VERSION_NUMBER / 100 % 100 == FKSTRING_VERSION_MINOR &&
		FKSTRING_VERSION_NUMBER % 100 == FKSTRING_VERSION_PATCH,
		"FKSTRING_VERSION_NUMBER is %d", FKSTRING_VERSION_NUMBER);
	return 1;
}

/* PACKAGE_VERSION comes from AC_INIT in configure.ac, via DEFS. */
static int test_version_matches_configure(char *errbuf, size_t errbuflen)
{
#ifdef PACKAGE_VERSION
	CHECK(strcmp(PACKAGE_VERSION, FKSTRING_VERSION) == 0,
		"configure.ac says '%s', fkstring.h says '%s'",
		PACKAGE_VERSION, FKSTRING_VERSION);
#endif
	return 1;
}

static test_case fkstrversion_tests[] = {
	{ "fkstrversion() matches FKSTRING_VERSION", test_fkstrversion_matches_header },
	{ "FKSTRING_VERSION matches MAJOR.MINOR.PATCH", test_version_string_matches_components },
	{ "FKSTRING_VERSION_NUMBER matches MAJOR/MINOR/PATCH", test_version_number_matches_components },
	{ "FKSTRING_VERSION matches configure's PACKAGE_VERSION", test_version_matches_configure },
};

test_suite fkstrversion_suite = { fkstrversion_tests, sizeof(fkstrversion_tests) / sizeof(fkstrversion_tests[0]) };
