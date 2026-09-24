#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fkstring.h>
#include <fkstring_internal.h>
#include "framework.h"

/*
 * Creates an unlinked temp file holding len bytes of buf, with its offset
 * rewound to 0. If pathbuf is non-NULL the file is left linked and its name
 * copied there (pathbuf must hold at least 32 bytes), for fkslurpfile().
 */
static int make_tempfile(const void *buf, size_t len, char *pathbuf)
{
	char	path[] = "/tmp/fkslurpXXXXXX";
	int	fd;

	if ((fd = mkstemp(path)) < 0)
		return -1;
	if (pathbuf)
		strcpy(pathbuf, path);
	else
		unlink(path);
	if (write(fd, buf, len) != (ssize_t)len || lseek(fd, 0, SEEK_SET) != 0)
	{
		close(fd);
		return -1;
	}
	return fd;
}

static int test_fkslurp_regular_file_sized_by_hint(char *errbuf, size_t errbuflen)
{
	static const char	data[] = "abc\0def\nghi";
	int			fd;
	fkstring		*s;

	CHECK((fd = make_tempfile(data, sizeof(data) - 1, NULL)) >= 0, "temp file setup failed");
	s = fkslurp(fd);
	close(fd);
	CHECK(s != NULL, "fkslurp() returned NULL");
	CHECK(fkstrlen(s) == sizeof(data) - 1, "expected len %zu, got %zu", sizeof(data) - 1, fkstrlen(s));
	CHECK(memcmp(fkcstr(s), data, sizeof(data)) == 0, "contents differ (embedded NUL lost?)");
	/* The fstat() hint plus the spare byte for the EOF-confirming read(). */
	CHECK(fkstrsize(s) == sizeof(data) + 1, "expected alloc %zu (size + 2), got %zu", sizeof(data) + 1, fkstrsize(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkslurp_starts_at_current_offset(char *errbuf, size_t errbuflen)
{
	int		fd;
	fkstring	*s;

	CHECK((fd = make_tempfile("0123456789", 10, NULL)) >= 0, "temp file setup failed");
	CHECK(lseek(fd, 4, SEEK_SET) == 4, "lseek() failed");
	s = fkslurp(fd);
	close(fd);
	CHECK(s != NULL, "fkslurp() returned NULL");
	CHECK(fkstrlen(s) == 6, "expected len 6, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "456789") == 0, "expected '456789', got '%s'", fkcstr(s));
	CHECK(fkstrsize(s) == 8, "expected alloc 8 (remaining size + 2), got %zu", fkstrsize(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkslurp_empty_file_honors_len_zero_invariant(char *errbuf, size_t errbuflen)
{
	int		fd;
	fkstring	*s;

	CHECK((fd = make_tempfile("", 0, NULL)) >= 0, "temp file setup failed");
	s = fkslurp(fd);
	close(fd);
	CHECK(s != NULL, "fkslurp() returned NULL");
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL");
	CHECK(fkstrsize(s) == 0, "expected alloc 0 per the len==0 invariant, got %zu", fkstrsize(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkslurp_pipe_releases_slack(char *errbuf, size_t errbuflen)
{
	int		fds[2];
	fkstring	*s;

	CHECK(pipe(fds) == 0, "pipe() failed");
	CHECK(write(fds[1], "abc", 3) == 3, "setup write failed");
	close(fds[1]);

	/* No size hint: starts from a _slurptry buffer, then fkfit()s it. */
	s = fkslurp(fds[0]);
	close(fds[0]);
	CHECK(s != NULL, "fkslurp() returned NULL");
	CHECK(fkstrlen(s) == 3, "expected len 3, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "abc") == 0, "expected 'abc', got '%s'", fkcstr(s));
	CHECK(fkstrsize(s) == 4, "expected alloc 4 after releasing the _slurptry slack, got %zu", fkstrsize(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkslurp_pipe_grows_past_slurptry(char *errbuf, size_t errbuflen)
{
	char		data[1000];
	int		fds[2];
	size_t		i, saved = _slurptry;
	fkstring	*s;

	for (i = 0; i < sizeof(data); i++)
		data[i] = (char)i;	/* includes embedded NULs */
	CHECK(pipe(fds) == 0, "pipe() failed");
	CHECK(write(fds[1], data, sizeof(data)) == sizeof(data), "setup write failed");
	close(fds[1]);

	_slurptry = 16;
	s = fkslurp(fds[0]);
	_slurptry = saved;
	close(fds[0]);
	CHECK(s != NULL, "fkslurp() returned NULL");
	CHECK(fkstrlen(s) == sizeof(data), "expected len %zu, got %zu", sizeof(data), fkstrlen(s));
	CHECK(memcmp(fkcstr(s), data, sizeof(data)) == 0, "contents differ");
	CHECK(fkcstr(s)[fkstrlen(s)] == '\0', "expected NUL terminator at cstr[len]");
	CHECK(fkstrsize(s) > fkstrlen(s), "expected alloc > len, got %zu", fkstrsize(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkslurp_empty_pipe_honors_len_zero_invariant(char *errbuf, size_t errbuflen)
{
	int		fds[2];
	fkstring	*s;

	CHECK(pipe(fds) == 0, "pipe() failed");
	close(fds[1]);
	s = fkslurp(fds[0]);
	close(fds[0]);
	CHECK(s != NULL, "fkslurp() returned NULL");
	CHECK(fkstrlen(s) == 0, "expected len 0, got %zu", fkstrlen(s));
	CHECK(fkcstr(s) == NULL, "expected cstr NULL");
	CHECK(fkstrsize(s) == 0, "expected alloc 0 per the len==0 invariant, got %zu", fkstrsize(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkslurp_proc_file_ignores_zero_size(char *errbuf, size_t errbuflen)
{
	int		fd;
	fkstring	*s;

	/* /proc files are regular files whose st_size is 0 despite having
	 * content, so a hint-trusting implementation would read nothing. */
	if ((fd = open("/proc/self/status", O_RDONLY)) < 0)
		return 1;	/* not Linux (or no /proc): nothing to test */
	s = fkslurp(fd);
	close(fd);
	CHECK(s != NULL, "fkslurp() returned NULL");
	CHECK(fkstrlen(s) > 0, "expected content from /proc/self/status, got len 0");
	CHECK(fkstartswithc(s, "Name:"), "expected content to start with 'Name:'");
	fkstrdestroy(s);
	return 1;
}

static int test_fkslurp_bad_fd_returns_null(char *errbuf, size_t errbuflen)
{
	int		fds[2];
	fkstring	*s;

	CHECK(pipe(fds) == 0, "pipe() failed");
	close(fds[0]);
	close(fds[1]);

	errno = 0;
	s = fkslurp(fds[0]);
	CHECK(s == NULL, "expected NULL on a closed fd, got non-NULL");
	CHECK(errno == EBADF, "expected errno EBADF, got %d", errno);
	return 1;
}

static int test_fkslurpfile_reads_whole_file(char *errbuf, size_t errbuflen)
{
	char		path[32];
	int		fd;
	fkstring	*s;

	CHECK((fd = make_tempfile("line1\nline2\n", 12, path)) >= 0, "temp file setup failed");
	close(fd);
	s = fkslurpfile(path);
	unlink(path);
	CHECK(s != NULL, "fkslurpfile() returned NULL");
	CHECK(fkstrlen(s) == 12, "expected len 12, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "line1\nline2\n") == 0, "unexpected contents '%s'", fkcstr(s));
	fkstrdestroy(s);
	return 1;
}

static int test_fkslurpfile_missing_file_returns_null(char *errbuf, size_t errbuflen)
{
	errno = 0;
	CHECK(fkslurpfile("/nonexistent/fkslurp/test") == NULL, "expected NULL for a missing file");
	CHECK(errno == ENOENT, "expected errno ENOENT, got %d", errno);
	return 1;
}

static int test_fkslurpfile_null_path_returns_null(char *errbuf, size_t errbuflen)
{
	CHECK(fkslurpfile(NULL) == NULL, "expected NULL for a NULL path");
	return 1;
}

static test_case fkslurp_tests[] = {
	{ "fkslurp() of a regular file allocates once from the fstat() size hint", test_fkslurp_regular_file_sized_by_hint },
	{ "fkslurp() reads from the current offset, not from byte 0", test_fkslurp_starts_at_current_offset },
	{ "fkslurp() of an empty file honors the len==0 invariant", test_fkslurp_empty_file_honors_len_zero_invariant },
	{ "fkslurp() of a short pipe releases the _slurptry slack", test_fkslurp_pipe_releases_slack },
	{ "fkslurp() of a pipe grows past _slurptry via allocforlen()", test_fkslurp_pipe_grows_past_slurptry },
	{ "fkslurp() of an empty pipe honors the len==0 invariant", test_fkslurp_empty_pipe_honors_len_zero_invariant },
	{ "fkslurp() of a /proc file isn't fooled by its st_size of 0", test_fkslurp_proc_file_ignores_zero_size },
	{ "fkslurp() on a closed fd returns NULL with errno EBADF", test_fkslurp_bad_fd_returns_null },
	{ "fkslurpfile() reads a whole file by path", test_fkslurpfile_reads_whole_file },
	{ "fkslurpfile() of a missing file returns NULL with errno ENOENT", test_fkslurpfile_missing_file_returns_null },
	{ "fkslurpfile(NULL) returns NULL", test_fkslurpfile_null_path_returns_null },
};

test_suite fkslurp_suite = { fkslurp_tests, sizeof(fkslurp_tests) / sizeof(fkslurp_tests[0]) };
