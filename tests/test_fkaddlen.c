#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fkstring.h>
#include <fkstring_internal.h>
#include "framework.h"

static int test_fkaddlen_normal(char *errbuf, size_t errbuflen)
{
	CHECK(fkaddlen(0, 0) == 0, "expected 0 + 0 == 0");
	CHECK(fkaddlen(3, 4) == 7, "expected 3 + 4 == 7, got %zu", fkaddlen(3, 4));
	CHECK(fkaddlen(SIZE_MAX - 1, 1) == SIZE_MAX, "expected SIZE_MAX at the exact limit");
	CHECK(fkaddlen(0, SIZE_MAX) == SIZE_MAX, "expected 0 + SIZE_MAX == SIZE_MAX");
	return 1;
}

static int test_fkaddlen_overflow_panics(char *errbuf, size_t errbuflen)
{
	/* fkpanic() exits, so run the overflowing add in a child (see test_fkpanic.c). */
	int	pipefd[2];
	pid_t	pid;
	int	status;
	char	captured[256];
	ssize_t	n;

	CHECK(pipe(pipefd) == 0, "pipe() failed");
	pid = fork();
	CHECK(pid >= 0, "fork() failed");
	if (pid == 0)
	{
		close(pipefd[0]);
		dup2(pipefd[1], 2);
		close(pipefd[1]);
		fkaddlen(SIZE_MAX, 1);
		_exit(0);
	}
	close(pipefd[1]);
	n = read(pipefd[0], captured, sizeof(captured) - 1);
	captured[n < 0 ? 0 : n] = '\0';
	close(pipefd[0]);
	CHECK(waitpid(pid, &status, 0) == pid, "waitpid() failed");
	CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 253,
		"expected exit code 253, got status 0x%x", status);
	CHECK(strcmp(captured, errmsgs[FKSTRERR_OVERFLOW]) == 0,
		"expected stderr '%s', got '%s'", errmsgs[FKSTRERR_OVERFLOW], captured);
	return 1;
}

static test_case fkaddlen_tests[] = {
	{ "fkaddlen() returns the sum up to and including SIZE_MAX", test_fkaddlen_normal },
	{ "fkaddlen() past SIZE_MAX panics with FKSTRERR_OVERFLOW", test_fkaddlen_overflow_panics },
};

test_suite fkaddlen_suite = { fkaddlen_tests, sizeof(fkaddlen_tests) / sizeof(fkaddlen_tests[0]) };
