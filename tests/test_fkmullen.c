#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fkstring.h>
#include <fkstring_internal.h>
#include "framework.h"

static int test_fkmullen_normal(char *errbuf, size_t errbuflen)
{
	CHECK(fkmullen(0, 0) == 0, "expected 0 * 0 == 0");
	CHECK(fkmullen(0, SIZE_MAX) == 0, "expected 0 * SIZE_MAX == 0");
	CHECK(fkmullen(SIZE_MAX, 0) == 0, "expected SIZE_MAX * 0 == 0");
	CHECK(fkmullen(3, 4) == 12, "expected 3 * 4 == 12, got %zu", fkmullen(3, 4));
	CHECK(fkmullen(1, SIZE_MAX) == SIZE_MAX, "expected 1 * SIZE_MAX == SIZE_MAX");
	CHECK(fkmullen(SIZE_MAX / 3, 3) == SIZE_MAX / 3 * 3, "expected no panic just under the limit");
	return 1;
}

static int test_fkmullen_overflow_panics(char *errbuf, size_t errbuflen)
{
	/* fkpanic() exits, so run the overflowing multiply in a child (see test_fkpanic.c). */
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
		fkmullen(SIZE_MAX / 2 + 1, 2);
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

static test_case fkmullen_tests[] = {
	{ "fkmullen() returns the product up to SIZE_MAX, including zero factors", test_fkmullen_normal },
	{ "fkmullen() past SIZE_MAX panics with FKSTRERR_OVERFLOW", test_fkmullen_overflow_panics },
};

test_suite fkmullen_suite = { fkmullen_tests, sizeof(fkmullen_tests) / sizeof(fkmullen_tests[0]) };
