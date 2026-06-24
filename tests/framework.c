#include <stdio.h>
#include "framework.h"

int run_suites(test_suite *suites, int nsuites)
{
	int	num, passed, failed, i, j;
	char	errbuf[256];

	num = passed = failed = 0;

	for (i = 0; i < nsuites; i++)
	{
		for (j = 0; j < suites[i].count; j++)
		{
			test_case *tc = &suites[i].cases[j];

			num++;
			printf("#%d (%s)... ", num, tc->desc);
			fflush(stdout);

			errbuf[0] = '\0';
			if (tc->fn(errbuf, sizeof(errbuf)))
			{
				printf("PASS\n");
				passed++;
			}
			else
			{
				printf("FAIL%s%s\n", errbuf[0] ? " - " : "", errbuf);
				failed++;
			}
		}
	}

	printf("\n%d tests, %d passed, %d failed\n", num, passed, failed);
	return failed == 0;
}
