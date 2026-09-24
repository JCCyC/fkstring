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
 * Creates a temp file holding len bytes of buf and copies its name to
 * pathbuf (at least 32 bytes). The caller unlink()s it.
 */
static int make_tempfile(const void *buf, size_t len, char *pathbuf)
{
	char	path[] = "/tmp/fkcatXXXXXX";
	int	fd;

	if ((fd = mkstemp(path)) < 0)
		return 0;
	strcpy(pathbuf, path);
	if (write(fd, buf, len) != (ssize_t)len)
	{
		close(fd);
		unlink(path);
		return 0;
	}
	close(fd);
	return 1;
}

static int test_fkcatfd_copies_file_to_fd(char *errbuf, size_t errbuflen)
{
	static const char	data[] = "abc\0def\nghi";
	char			path[32], buf[32];
	int			fds[2];
	ssize_t			n;

	CHECK(make_tempfile(data, sizeof(data) - 1, path), "temp file setup failed");
	CHECK(pipe(fds) == 0, "pipe() failed");
	n = fkcatfd(path, fds[1]);
	unlink(path);
	close(fds[1]);
	CHECK(n == sizeof(data) - 1, "expected %zu bytes copied, got %zd", sizeof(data) - 1, n);
	n = read(fds[0], buf, sizeof(buf));
	close(fds[0]);
	CHECK(n == sizeof(data) - 1, "expected to read back %zu bytes, got %zd", sizeof(data) - 1, n);
	CHECK(memcmp(buf, data, sizeof(data) - 1) == 0, "contents differ (embedded NUL lost?)");
	return 1;
}

static int test_fkcatfd_streams_in_chunks(char *errbuf, size_t errbuflen)
{
	char		data[1000], buf[1100], path[32];
	int		fds[2];
	size_t		i, got = 0, saved = _catbufsize;
	ssize_t		n;

	for (i = 0; i < sizeof(data); i++)
		data[i] = (char)i;
	CHECK(make_tempfile(data, sizeof(data), path), "temp file setup failed");
	CHECK(pipe(fds) == 0, "pipe() failed");

	/* 1000 isn't a multiple of 7, so the last chunk is a short read. */
	_catbufsize = 7;
	n = fkcatfd(path, fds[1]);
	_catbufsize = saved;
	unlink(path);
	close(fds[1]);
	CHECK(n == sizeof(data), "expected %zu bytes copied, got %zd", sizeof(data), n);
	while ((n = read(fds[0], buf + got, sizeof(buf) - got)) > 0)
		got += (size_t)n;
	close(fds[0]);
	CHECK(got == sizeof(data), "expected to read back %zu bytes, got %zu", sizeof(data), got);
	CHECK(memcmp(buf, data, sizeof(data)) == 0, "contents differ across chunk boundaries");
	return 1;
}

static int test_fkcatfd_empty_file(char *errbuf, size_t errbuflen)
{
	char	path[32], buf[8];
	int	fds[2];
	ssize_t	n;

	CHECK(make_tempfile("", 0, path), "temp file setup failed");
	CHECK(pipe(fds) == 0, "pipe() failed");
	n = fkcatfd(path, fds[1]);
	unlink(path);
	close(fds[1]);
	CHECK(n == 0, "expected 0 bytes copied, got %zd", n);
	CHECK(read(fds[0], buf, sizeof(buf)) == 0, "expected nothing readable");
	close(fds[0]);
	return 1;
}

static int test_fkcatfd_missing_file(char *errbuf, size_t errbuflen)
{
	ssize_t	n;

	errno = 0;
	n = fkcatfd("/nonexistent/fkcat/test", 1);
	CHECK(n == -1, "expected -1 for a missing file, got %zd", n);
	CHECK(errno == ENOENT, "expected errno ENOENT, got %d", errno);
	return 1;
}

static int test_fkcatfd_directory(char *errbuf, size_t errbuflen)
{
	int	fds[2];
	ssize_t	n;

	/* open() succeeds on a directory; it's read() that fails. */
	CHECK(pipe(fds) == 0, "pipe() failed");
	errno = 0;
	n = fkcatfd("/", fds[1]);
	close(fds[0]);
	close(fds[1]);
	CHECK(n == -1, "expected -1 for a directory, got %zd", n);
	CHECK(errno == EISDIR, "expected errno EISDIR, got %d", errno);
	return 1;
}

static int test_fkcatfd_bad_fd(char *errbuf, size_t errbuflen)
{
	char	path[32];
	int	fds[2];
	ssize_t	n;

	CHECK(make_tempfile("data", 4, path), "temp file setup failed");
	CHECK(pipe(fds) == 0, "pipe() failed");
	close(fds[0]);
	close(fds[1]);
	errno = 0;
	n = fkcatfd(path, fds[1]);
	unlink(path);
	CHECK(n == -1, "expected -1 on a closed fd, got %zd", n);
	CHECK(errno == EBADF, "expected errno EBADF, got %d", errno);
	return 1;
}

static int test_fkcatfd_null_path(char *errbuf, size_t errbuflen)
{
	ssize_t	n;

	errno = 0;
	n = fkcatfd(NULL, 1);
	CHECK(n == -1, "expected -1 for a NULL path, got %zd", n);
	CHECK(errno == EINVAL, "expected errno EINVAL, got %d", errno);
	return 1;
}

static int test_fkcat_writes_to_stdout(char *errbuf, size_t errbuflen)
{
	char	path[32], buf[16];
	int	fds[2], saved_stdout;
	ssize_t	n;

	CHECK(make_tempfile("to stdout\n", 10, path), "temp file setup failed");
	CHECK(pipe(fds) == 0, "pipe() failed");
	fflush(stdout);
	CHECK((saved_stdout = dup(STDOUT_FILENO)) >= 0, "dup() failed");
	CHECK(dup2(fds[1], STDOUT_FILENO) == STDOUT_FILENO, "dup2() failed");
	n = fkcat(path);
	dup2(saved_stdout, STDOUT_FILENO);
	close(saved_stdout);
	close(fds[1]);
	unlink(path);
	CHECK(n == 10, "expected 10 bytes copied, got %zd", n);
	n = read(fds[0], buf, sizeof(buf));
	close(fds[0]);
	CHECK(n == 10 && memcmp(buf, "to stdout\n", 10) == 0, "expected 'to stdout\\n' on fd 1");
	return 1;
}

static int test_fkcatf_copies_file_to_stream(char *errbuf, size_t errbuflen)
{
	static const char	data[] = "line1\n\0line2\n";
	char			path[32], buf[32];
	FILE			*f;
	ssize_t			n;
	size_t			got;

	CHECK(make_tempfile(data, sizeof(data) - 1, path), "temp file setup failed");
	CHECK((f = tmpfile()) != NULL, "tmpfile() failed");
	CHECK(fputs("prefix:", f) >= 0, "fputs() failed");
	n = fkcatf(path, f);
	unlink(path);
	CHECK(n == sizeof(data) - 1, "expected %zu bytes copied, got %zd", sizeof(data) - 1, n);
	rewind(f);
	got = fread(buf, 1, sizeof(buf), f);
	fclose(f);
	CHECK(got == 7 + sizeof(data) - 1, "expected %zu bytes in the stream, got %zu", 7 + sizeof(data) - 1, got);
	CHECK(memcmp(buf, "prefix:", 7) == 0, "earlier buffered output was lost or reordered");
	CHECK(memcmp(buf + 7, data, sizeof(data) - 1) == 0, "contents differ (embedded NUL lost?)");
	return 1;
}

static int test_fkcatf_read_only_stream(char *errbuf, size_t errbuflen)
{
	char	path[32];
	FILE	*f;
	ssize_t	n;

	CHECK(make_tempfile("data", 4, path), "temp file setup failed");
	CHECK((f = fopen("/dev/null", "r")) != NULL, "fopen() failed");
	n = fkcatf(path, f);
	fclose(f);
	unlink(path);
	CHECK(n == -1, "expected -1 writing to a read-only stream, got %zd", n);
	return 1;
}

static int test_fkcatf_null_args(char *errbuf, size_t errbuflen)
{
	ssize_t	n;

	errno = 0;
	n = fkcatf("/etc/passwd", NULL);
	CHECK(n == -1, "expected -1 for a NULL stream, got %zd", n);
	CHECK(errno == EINVAL, "expected errno EINVAL for a NULL stream, got %d", errno);
	errno = 0;
	n = fkcatf(NULL, stdout);
	CHECK(n == -1, "expected -1 for a NULL path, got %zd", n);
	CHECK(errno == EINVAL, "expected errno EINVAL for a NULL path, got %d", errno);
	return 1;
}

static test_case fkcat_tests[] = {
	{ "fkcatfd() copies a file, embedded NULs included, and returns its size", test_fkcatfd_copies_file_to_fd },
	{ "fkcatfd() streams correctly across many _catbufsize chunks", test_fkcatfd_streams_in_chunks },
	{ "fkcatfd() of an empty file writes nothing and returns 0", test_fkcatfd_empty_file },
	{ "fkcatfd() of a missing file returns -1 with errno ENOENT", test_fkcatfd_missing_file },
	{ "fkcatfd() of a directory returns -1 with errno EISDIR", test_fkcatfd_directory },
	{ "fkcatfd() to a closed fd returns -1 with errno EBADF", test_fkcatfd_bad_fd },
	{ "fkcatfd(NULL, fd) returns -1 with errno EINVAL", test_fkcatfd_null_path },
	{ "fkcat() writes the file to fd 1", test_fkcat_writes_to_stdout },
	{ "fkcatf() appends a file to a FILE after its buffered output", test_fkcatf_copies_file_to_stream },
	{ "fkcatf() to a read-only stream returns -1", test_fkcatf_read_only_stream },
	{ "fkcatf() with a NULL path or stream returns -1 with errno EINVAL", test_fkcatf_null_args },
};

test_suite fkcat_suite = { fkcat_tests, sizeof(fkcat_tests) / sizeof(fkcat_tests[0]) };
