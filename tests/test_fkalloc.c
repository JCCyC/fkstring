#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fkstring.h>
#include <fkstring_internal.h>
#include "framework.h"

static int test_fkalloc_zero(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkalloc(0);

	CHECK(s != NULL, "fkalloc(0) returned NULL");
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 0, "expected alloc 0, got %zu", fkstrsize(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL, got non-NULL");
	fkstrdestroy(s);
	return 1;
}

static int test_fkalloc_normal(char *errbuf, size_t errbuflen)
{
	fkstring *s = fkalloc(11);

	CHECK(s != NULL, "fkalloc(11) returned NULL");
	CHECK(fkstrlen(s) == 11, "expected len 11, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 16, "expected alloc 16 (allocforlen(11)), got %zu", fkstrsize(s));
	CHECK(fkcstr(s) != NULL, "expected non-NULL cstr");
	CHECK(fkcstr(s)[11] == '\0', "expected NUL terminator at cstr[len]");
	fkstrdestroy(s);
	return 1;
}

static int test_fkalloc_fill_and_use(char *errbuf, size_t errbuflen)
{
	/* The buffer is meant to be filled in by the caller, then used normally. */
	fkstring *s = fkalloc(5);

	memcpy(fkcstr(s), "hello", 5);
	CHECK(strcmp(fkcstr(s), "hello") == 0, "expected 'hello', got '%s'", fkcstr(s));
	fkstrcatc(s, " world");
	CHECK(fkstrlen(s) == 11, "expected len 11 after append, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "hello world") == 0, "expected 'hello world', got '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkcalloc_zero(char *errbuf, size_t errbuflen)
{
	fkstring *a = fkcalloc(0, 10);
	fkstring *b = fkcalloc(10, 0);

	CHECK(fkstrlen(a) == 0 && fkstrsize(a) == 0 && fkcstr(a) == NULL,
		"fkcalloc(0, 10) should be empty with alloc 0 and cstr NULL");
	CHECK(fkstrlen(b) == 0 && fkstrsize(b) == 0 && fkcstr(b) == NULL,
		"fkcalloc(10, 0) should be empty with alloc 0 and cstr NULL");
	fkstrdestroy(a);
	fkstrdestroy(b);
	return 1;
}

static int test_fkcalloc_zeroed(char *errbuf, size_t errbuflen)
{
	fkstring	*s = fkcalloc(25, 4);
	size_t		i;

	CHECK(fkstrlen(s) == 100, "expected len 100, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == allocforlen(100), "expected alloc %zu, got %zu",
		allocforlen(100), fkstrsize(s));
	for (i = 0; i <= 100; i++)
		CHECK(fkcstr(s)[i] == '\0', "expected zero byte at offset %zu", i);
	fkstrdestroy(s);
	return 1;
}

static int test_fkcalloc_embedded_nuls(char *errbuf, size_t errbuflen)
{
	/* len stays authoritative: the zero bytes are content, not a terminator. */
	fkstring *s = fkcalloc(3, 1);

	fkstrcatone(s, 'x');
	CHECK(fkstrlen(s) == 4, "expected len 4, got %zu", fkstrlen(s));
	CHECK(memcmp(fkcstr(s), "\0\0\0x", 5) == 0, "expected three NULs, 'x', then terminator");
	fkstrdestroy(s);
	return 1;
}

static int test_fkcalloc_overflow_panics(char *errbuf, size_t errbuflen)
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
		close(pipefd[0]);
		dup2(pipefd[1], 2);
		close(pipefd[1]);
		fkcalloc(SIZE_MAX / 2 + 1, 2);
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

static test_case fkalloc_tests[] = {
	{ "fkalloc(0) produces an empty fkstring honoring the len==0 invariant", test_fkalloc_zero },
	{ "fkalloc() sets len, alloc, and the NUL terminator at cstr[len]", test_fkalloc_normal },
	{ "fkalloc()'s buffer can be filled in and then appended to", test_fkalloc_fill_and_use },
	{ "fkcalloc() with nmemb or size 0 produces an empty fkstring", test_fkcalloc_zero },
	{ "fkcalloc() zeroes all nmemb * size bytes plus the terminator", test_fkcalloc_zeroed },
	{ "fkcalloc()'s zero bytes count toward len as embedded NULs", test_fkcalloc_embedded_nuls },
	{ "fkcalloc() with nmemb * size past SIZE_MAX panics with FKSTRERR_OVERFLOW", test_fkcalloc_overflow_panics },
};

test_suite fkalloc_suite = { fkalloc_tests, sizeof(fkalloc_tests) / sizeof(fkalloc_tests[0]) };
