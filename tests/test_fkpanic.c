#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fkstring.h>
#include <fkstring_internal.h>
#include "framework.h"

/*
 * fkpanic() calls exit(253) unconditionally, so every case here forks: the
 * child invokes fkpanic() with its stderr redirected to a pipe, and the
 * parent checks the child's exit status plus whatever it wrote.
 */

static int run_fkpanic_child(int cause, int *exitstatus, char *captured, size_t capturedlen)
{
	int	pipefd[2];
	pid_t	pid;
	ssize_t	n;
	int	status;

	if (pipe(pipefd) != 0)
		return 0;

	pid = fork();
	if (pid < 0)
		return 0;

	if (pid == 0)
	{
		close(pipefd[0]);
		dup2(pipefd[1], 2);
		close(pipefd[1]);
		fkpanic(cause);
		_exit(254); /* unreachable: fkpanic() never returns */
	}

	close(pipefd[1]);
	n = read(pipefd[0], captured, capturedlen - 1);
	if (n < 0)
		n = 0;
	captured[n] = '\0';
	close(pipefd[0]);

	if (waitpid(pid, &status, 0) != pid)
		return 0;

	*exitstatus = status;
	return 1;
}

static int test_fkpanic_memalloc(char *errbuf, size_t errbuflen)
{
	int	status;
	char	captured[256];

	CHECK(run_fkpanic_child(FKSTRERR_MEMALLOC, &status, captured, sizeof(captured)),
		"failed to fork/run child process");
	CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 253,
		"expected exit code 253, got status 0x%x", status);
	CHECK(strcmp(captured, errmsgs[FKSTRERR_MEMALLOC]) == 0,
		"expected stderr '%s', got '%s'", errmsgs[FKSTRERR_MEMALLOC], captured);
	return 1;
}

static int test_fkpanic_vsnprintf(char *errbuf, size_t errbuflen)
{
	int	status;
	char	captured[256];

	CHECK(run_fkpanic_child(FKSTRERR_VSNPRINTF, &status, captured, sizeof(captured)),
		"failed to fork/run child process");
	CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 253,
		"expected exit code 253, got status 0x%x", status);
	CHECK(strcmp(captured, errmsgs[FKSTRERR_VSNPRINTF]) == 0,
		"expected stderr '%s', got '%s'", errmsgs[FKSTRERR_VSNPRINTF], captured);
	return 1;
}

static int test_fkpanic_success_cause_prints_nothing(char *errbuf, size_t errbuflen)
{
	int	status;
	char	captured[256];

	/* cause == FKSTRERR_SUCCESS (0) skips the "if (cause)" guard, so no
	 * message should be written even though the process still exits 253. */
	CHECK(run_fkpanic_child(FKSTRERR_SUCCESS, &status, captured, sizeof(captured)),
		"failed to fork/run child process");
	CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 253,
		"expected exit code 253, got status 0x%x", status);
	CHECK(captured[0] == '\0',
		"expected no stderr output, got '%s'", captured);
	return 1;
}

static test_case fkpanic_tests[] = {
	{ "fkpanic(FKSTRERR_MEMALLOC) exits 253 and prints the alloc error message", test_fkpanic_memalloc },
	{ "fkpanic(FKSTRERR_VSNPRINTF) exits 253 and prints the vsnprintf error message", test_fkpanic_vsnprintf },
	{ "fkpanic(FKSTRERR_SUCCESS) exits 253 but prints nothing", test_fkpanic_success_cause_prints_nothing },
};

test_suite fkpanic_suite = { fkpanic_tests, sizeof(fkpanic_tests) / sizeof(fkpanic_tests[0]) };
