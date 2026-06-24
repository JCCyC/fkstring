#include "framework.h"

extern test_suite allocforlen_suite;
extern test_suite fkpanic_suite;
extern test_suite fkstrerr_suite;
extern test_suite fkstrnew_suite;
extern test_suite fkstrdestroy_suite;
extern test_suite fkstrdup_suite;
extern test_suite fkstrtrunc_suite;
extern test_suite fkstrcat_suite;
extern test_suite fksubstr_suite;
extern test_suite fkremove_suite;
extern test_suite fktrim_suite;
extern test_suite fksplit_suite;
extern test_suite fksprintf_suite;
extern test_suite fkstrwrite_suite;
extern test_suite fkstrread_suite;

int main(void)
{
	test_suite suites[] = {
		fkstrnew_suite,
		fkstrdestroy_suite,
		fkstrdup_suite,
		fkstrcat_suite,
		fksubstr_suite,
		fkstrtrunc_suite,
		fkremove_suite,
		fktrim_suite,
		fksplit_suite,
		allocforlen_suite,
		fkpanic_suite,
		fkstrerr_suite,
		fksprintf_suite,
		fkstrwrite_suite,
		fkstrread_suite,
	};

	return run_suites(suites, sizeof(suites) / sizeof(suites[0])) ? 0 : 1;
}
