#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fkstring.h>
#include <fkstring_internal.h>
#include "framework.h"

static int test_fkslack_null(char *errbuf, size_t errbuflen)
{
	CHECK(fkslack(NULL, 10) == NULL, "fkslack(NULL, 10) should return NULL");
	return 1;
}

static int test_fkslack_empty_noop(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("");

	CHECK(fkslack(s, 100) == s, "fkslack() should return its argument");
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 0, "expected alloc 0 (len==0 invariant), got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL (len==0 invariant), got non-NULL");
	fkstrdestroy(s);
	return 1;
}

static int test_fkslack_grows_exactly(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("hello");

	CHECK(fkslack(s, 100) == s, "fkslack() should return its argument");
	CHECK(fkstrsize(s) == 5 + 100 + 1, "expected alloc 106, got %zu", fkstrsize(s));
	CHECK(fkstrlen(s) == 5, "expected len 5, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "hello") == 0, "expected 'hello', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkslack_enough_already(char *errbuf, size_t errbuflen)
{
	/* allocforlen(5) is 16, so 10 bytes of slack are already there. */
	fkstring	*s = fkstrnew("hello");
	char		*before = fkcstr(s);
	size_t		alloc = fkstrsize(s);

	fkslack(s, 10);
	CHECK(fkstrsize(s) == alloc, "expected alloc to stay %zu, got %zu", alloc, fkstrsize(s));
	CHECK(fkcstr(s) == before, "buffer should not have been reallocated");
	fkslack(s, 0);
	CHECK(fkstrsize(s) == alloc, "fkslack(s, 0) changed alloc to %zu", fkstrsize(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkslack_appends_fit(char *errbuf, size_t errbuflen)
{
	/* The point of fkslack(): appends within the slack never reallocate. */
	fkstring	*s = fkstrnew("x");
	char		*before;
	int		i;

	fkslack(s, 1000);
	before = fkcstr(s);
	for (i = 0; i < 1000; i++)
		fkstrcatone(s, 'y');
	CHECK(fkstrlen(s) == 1001, "expected len 1001, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 1002, "expected alloc to stay 1002, got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == before, "appends within the slack reallocated the buffer");
	fkstrdestroy(s);
	return 1;
}

static int test_fkslack_undone_by_trunc(char *errbuf, size_t errbuflen)
{
	/* Documented, accepted behavior: no per-string floor survives fkstrtrunc(). */
	fkstring *s = fkstrnew("hello world");

	fkslack(s, 1000);
	fkstrtrunc(s, 5);
	CHECK(fkstrsize(s) == _minalloc, "expected fkstrtrunc() to deflate alloc to %zu, got %zu",
		_minalloc, fkstrsize(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkslack_overflow_panics(char *errbuf, size_t errbuflen)
{
	/* fkpanic() exits, so run the overflowing call in a child (see test_fkpanic.c). */
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
		fkstring *s = fkstrnew("hello");

		close(pipefd[0]);
		dup2(pipefd[1], 2);
		close(pipefd[1]);
		fkslack(s, SIZE_MAX - 5);
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

static int test_fkfit_null(char *errbuf, size_t errbuflen)
{
	CHECK(fkfit(NULL) == NULL, "fkfit(NULL) should return NULL");
	return 1;
}

static int test_fkfit_empty_noop(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("");

	CHECK(fkfit(s) == s, "fkfit() should return its argument");
	CHECK(fkstrlen(s) == 0 && fkstrsize(s) == 0 && fkcstr(s) == NULL,
		"fkfit() on an empty string should keep len 0, alloc 0, cstr NULL");
	fkstrdestroy(s);
	return 1;
}

static int test_fkfit_below_minalloc(char *errbuf, size_t errbuflen)
{
	/* fkstrnew("abc") gets _minalloc bytes; fkfit() ignores that floor. */
	fkstring *s = fkstrnew("abc");

	CHECK(fkstrsize(s) == _minalloc, "precondition: expected alloc %zu, got %zu",
		_minalloc, fkstrsize(s));
	CHECK(fkfit(s) == s, "fkfit() should return its argument");
	CHECK(fkstrsize(s) == 4, "expected alloc 4, got %zu", fkstrsize(s));
	CHECK(strcmp(fkcstr(s), "abc") == 0, "expected 'abc', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkfit_undoes_slack(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnew("hello world");

	fkslack(s, 500);
	fkfit(s);
	CHECK(fkstrsize(s) == 12, "expected alloc 12, got %zu", fkstrsize(s));
	CHECK(fkstrlen(s) == 11, "expected len 11, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "hello world") == 0, "expected 'hello world', got '%s'", fkcstr(s));
	fkfit(s);
	CHECK(fkstrsize(s) == 12, "second fkfit() changed alloc to %zu", fkstrsize(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkfit_embedded_nuls(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkstrnewb("a\0b\0c", 5);

	fkfit(s);
	CHECK(fkstrlen(s) == 5, "expected len 5, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 6, "expected alloc 6, got %zu", fkstrsize(s));
	CHECK(memcmp(fkcstr(s), "a\0b\0c", 6) == 0, "contents or terminator changed");
	fkstrdestroy(s);
	return 1;
}

static int test_fkfit_then_append(char *errbuf, size_t errbuflen)
{
	/* A fitted string has no slack, so the next append must grow it. */
	fkstring *s = fkstrnew("abc");

	fkfit(s);
	fkstrcatc(s, "def");
	CHECK(fkstrlen(s) == 6, "expected len 6, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) > 6, "expected alloc > 6, got %zu", fkstrsize(s));
	CHECK(strcmp(fkcstr(s), "abcdef") == 0, "expected 'abcdef', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static test_case fkslack_tests[] = {
	{ "fkslack(NULL, n) returns NULL", test_fkslack_null },
	{ "fkslack() on an empty string is a no-op honoring the len==0 invariant", test_fkslack_empty_noop },
	{ "fkslack() grows alloc to exactly len + n + 1, keeping the contents", test_fkslack_grows_exactly },
	{ "fkslack() leaves the buffer alone when the slack is already there", test_fkslack_enough_already },
	{ "appends within fkslack()'s slack don't reallocate", test_fkslack_appends_fit },
	{ "fkstrtrunc() past the _deflatefactor threshold undoes fkslack()", test_fkslack_undone_by_trunc },
	{ "fkslack() with len + n past SIZE_MAX panics with FKSTRERR_OVERFLOW", test_fkslack_overflow_panics },
	{ "fkfit(NULL) returns NULL", test_fkfit_null },
	{ "fkfit() on an empty string is a no-op honoring the len==0 invariant", test_fkfit_empty_noop },
	{ "fkfit() shrinks alloc to len + 1, below _minalloc", test_fkfit_below_minalloc },
	{ "fkfit() reclaims fkslack()'s slack and is idempotent", test_fkfit_undoes_slack },
	{ "fkfit() preserves embedded NULs and the terminator", test_fkfit_embedded_nuls },
	{ "a fitted string grows normally on the next append", test_fkfit_then_append },
};

test_suite fkslack_suite = { fkslack_tests, sizeof(fkslack_tests) / sizeof(fkslack_tests[0]) };
