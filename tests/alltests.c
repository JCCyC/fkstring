#include "framework.h"

extern test_suite allocforlen_suite;
extern test_suite fkaddlen_suite;
extern test_suite fkpanic_suite;
extern test_suite fkstrerr_suite;
extern test_suite fkstrnew_suite;
extern test_suite fkstrdestroy_suite;
extern test_suite fkstrdup_suite;
extern test_suite fkstrtrunc_suite;
extern test_suite fkstrcat_suite;
extern test_suite fkinsert_suite;
extern test_suite fksubstr_suite;
extern test_suite fkremove_suite;
extern test_suite fktrim_suite;
extern test_suite fksplit_suite;
extern test_suite fkjoin_suite;
extern test_suite fkstrcmp_suite;
extern test_suite fkstrfind_suite;
extern test_suite fkstrchr_suite;
extern test_suite fkstartswith_suite;
extern test_suite fkreplace_suite;
extern test_suite fksprintf_suite;
extern test_suite fkstrcatf_suite;
extern test_suite fkstrwrite_suite;
extern test_suite fkstrread_suite;

int main(void)
{
	test_suite suites[] = {
		fkstrnew_suite,
		fkstrdestroy_suite,
		fkstrdup_suite,
		fkstrcat_suite,
		fkinsert_suite,
		fksubstr_suite,
		fkstrtrunc_suite,
		fkremove_suite,
		fktrim_suite,
		fksplit_suite,
		fkjoin_suite,
		fkstrcmp_suite,
		fkstrfind_suite,
		fkstrchr_suite,
		fkstartswith_suite,
		fkreplace_suite,
		allocforlen_suite,
		fkaddlen_suite,
		fkpanic_suite,
		fkstrerr_suite,
		fksprintf_suite,
		fkstrcatf_suite,
		fkstrwrite_suite,
		fkstrread_suite,
	};

	return run_suites(suites, sizeof(suites) / sizeof(suites[0])) ? 0 : 1;
}
