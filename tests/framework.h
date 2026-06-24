#ifndef __FKSTRING_TESTS_FRAMEWORK_H__
#define __FKSTRING_TESTS_FRAMEWORK_H__

#include <stdio.h>
#include <stddef.h>

typedef int (*test_fn)(char *errbuf, size_t errbuflen);

typedef struct
{
	const char	*desc;
	test_fn		fn;
} test_case;

typedef struct
{
	test_case	*cases;
	int		count;
} test_suite;

/* Relies on a test_fn's parameters being named errbuf/errbuflen. */
#define CHECK(cond, ...) \
	do { \
		if (!(cond)) \
		{ \
			snprintf(errbuf, errbuflen, __VA_ARGS__); \
			return 0; \
		} \
	} while (0)

int run_suites(test_suite *suites, int nsuites);

#endif /* __FKSTRING_TESTS_FRAMEWORK_H__ */
