#include <string.h>
#include <unistd.h>
#include <fkstring.h>
#include "framework.h"

static int test_fkstrread_partial(char *errbuf, size_t errbuflen)
{
	int		fds[2];
	fkstring	*s;

	CHECK(pipe(fds) == 0, "pipe() failed");
	CHECK(write(fds[1], "abc", 3) == 3, "setup write failed");
	close(fds[1]);

	s = fkstrread(fds[0], 10);
	CHECK(s != NULL, "fkstrread() returned NULL");
	CHECK(fkstrlen(s) == 3, "expected len 3, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 11, "expected alloc 11 (count+1), got %zu", fkstrsize(s));
	CHECK(strcmp(fkcstr(s), "abc") == 0, "expected 'abc', got '%s'", fkcstr(s));
	close(fds[0]);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrread_exact(char *errbuf, size_t errbuflen)
{
	int		fds[2];
	fkstring	*s;

	CHECK(pipe(fds) == 0, "pipe() failed");
	CHECK(write(fds[1], "abcde", 5) == 5, "setup write failed");
	close(fds[1]);

	s = fkstrread(fds[0], 5);
	CHECK(fkstrlen(s) == 5, "expected len 5, got %zu", fkstrlen(s));
	CHECK(fkstrsize(s) == 6, "expected alloc 6 (count+1), got %zu", fkstrsize(s));
	CHECK(strcmp(fkcstr(s), "abcde") == 0, "expected 'abcde', got '%s'", fkcstr(s));
	close(fds[0]);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrread_eof_honors_len_zero_invariant(char *errbuf, size_t errbuflen)
{
	int		fds[2];
	fkstring	*s;

	/* Close the write end with nothing written: the read end hits EOF
	 * (read() returns 0) immediately. */
	CHECK(pipe(fds) == 0, "pipe() failed");
	close(fds[1]);

	s = fkstrread(fds[0], 10);
	CHECK(s != NULL, "fkstrread() returned NULL");
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL");
	CHECK(fkstrsize(s) == 0, "expected alloc 0 per the len==0 invariant, got %zu", fkstrsize(s));
	close(fds[0]);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrread_zero_count_honors_len_zero_invariant(char *errbuf, size_t errbuflen)
{
	int		fds[2];
	fkstring	*s;

	/* A request for 0 bytes takes the same "bytesread == 0" branch as
	 * EOF, even though the pipe is open and has nothing to do with EOF. */
	CHECK(pipe(fds) == 0, "pipe() failed");

	s = fkstrread(fds[0], 0);
	CHECK(s != NULL, "fkstrread() returned NULL");
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL");
	CHECK(fkstrsize(s) == 0, "expected alloc 0 per the len==0 invariant, got %zu", fkstrsize(s));
	close(fds[0]);
	close(fds[1]);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrread_bad_fd_returns_null(char *errbuf, size_t errbuflen)
{
	int		fds[2];
	int		badfd;
	fkstring	*s;

	CHECK(pipe(fds) == 0, "pipe() failed");
	badfd = fds[0];
	close(fds[0]);
	close(fds[1]);

	s = fkstrread(badfd, 10);
	CHECK(s == NULL, "expected NULL on a closed fd, got non-NULL");
	return 1;
}

static test_case fkstrread_tests[] = {
	{ "fkstrread() of a partial read sets len/alloc correctly", test_fkstrread_partial },
	{ "fkstrread() requesting exactly the available bytes reads them all", test_fkstrread_exact },
	{ "fkstrread() at EOF honors the len==0 invariant", test_fkstrread_eof_honors_len_zero_invariant },
	{ "fkstrread() with count==0 honors the len==0 invariant", test_fkstrread_zero_count_honors_len_zero_invariant },
	{ "fkstrread() on a closed fd returns NULL", test_fkstrread_bad_fd_returns_null },
};

test_suite fkstrread_suite = { fkstrread_tests, sizeof(fkstrread_tests) / sizeof(fkstrread_tests[0]) };
