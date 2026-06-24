#include <string.h>
#include <unistd.h>
#include <fkstring.h>
#include "framework.h"

static int test_fkstrwrite_roundtrip(char *errbuf, size_t errbuflen)
{
	int		fds[2];
	fkstring	*s = fkstrnew("hello, pipe");
	ssize_t		written;
	char		buf[32];
	ssize_t		nread;

	CHECK(pipe(fds) == 0, "pipe() failed");
	written = fkstrwrite(fds[1], s);
	CHECK(written == (ssize_t)fkstrlen(s), "expected %zu bytes written, got %zd", fkstrlen(s), written);
	nread = read(fds[0], buf, sizeof(buf));
	CHECK(nread == written, "expected to read back %zd bytes, got %zd", written, nread);
	CHECK(memcmp(buf, "hello, pipe", 11) == 0, "content mismatch on readback");
	close(fds[0]);
	close(fds[1]);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrwrite_null_fks(char *errbuf, size_t errbuflen)
{
	int	fds[2];
	ssize_t	written;
	char	buf[8];

	CHECK(pipe(fds) == 0, "pipe() failed");
	written = fkstrwrite(fds[1], NULL);
	CHECK(written == 0, "expected 0 bytes written, got %zd", written);
	close(fds[1]);
	CHECK(read(fds[0], buf, sizeof(buf)) == 0, "expected nothing readable (write end closed, no data)");
	close(fds[0]);
	return 1;
}

static int test_fkstrwrite_empty_fks(char *errbuf, size_t errbuflen)
{
	int		fds[2];
	fkstring	*s = fkstrnew(NULL);
	ssize_t		written;
	char		buf[8];

	CHECK(pipe(fds) == 0, "pipe() failed");
	written = fkstrwrite(fds[1], s);
	CHECK(written == 0, "expected 0 bytes written, got %zd", written);
	close(fds[1]);
	CHECK(read(fds[0], buf, sizeof(buf)) == 0, "expected nothing readable (write end closed, no data)");
	close(fds[0]);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrwrite_bad_fd(char *errbuf, size_t errbuflen)
{
	int		fds[2];
	int		badfd;
	fkstring	*s = fkstrnew("data");
	ssize_t		written;

	CHECK(pipe(fds) == 0, "pipe() failed");
	badfd = fds[1];
	close(fds[0]);
	close(fds[1]);

	written = fkstrwrite(badfd, s);
	CHECK(written == -1, "expected -1 on a closed fd, got %zd", written);
	fkstrdestroy(s);
	return 1;
}

static int test_fkstrwrite_embedded_nul(char *errbuf, size_t errbuflen)
{
	static const char	raw[] = { 'a', '\0', 'b', 'c' };
	int			fds[2];
	fkstring		*s = fkstrnewb(raw, sizeof(raw));
	ssize_t			written;
	char			buf[8];

	CHECK(pipe(fds) == 0, "pipe() failed");
	written = fkstrwrite(fds[1], s);
	CHECK(written == 4, "expected 4 bytes written (length-authoritative, not strlen), got %zd", written);
	CHECK(read(fds[0], buf, sizeof(buf)) == 4, "expected to read back 4 bytes");
	CHECK(memcmp(buf, raw, 4) == 0, "content mismatch around embedded NUL");
	close(fds[0]);
	close(fds[1]);
	fkstrdestroy(s);
	return 1;
}

static test_case fkstrwrite_tests[] = {
	{ "fkstrwrite() writes len bytes that round-trip exactly through a pipe", test_fkstrwrite_roundtrip },
	{ "fkstrwrite() with a NULL fks writes zero bytes and returns 0", test_fkstrwrite_null_fks },
	{ "fkstrwrite() with an empty fks writes zero bytes and returns 0", test_fkstrwrite_empty_fks },
	{ "fkstrwrite() on a closed fd returns -1", test_fkstrwrite_bad_fd },
	{ "fkstrwrite() writes embedded NUL bytes rather than stopping at them", test_fkstrwrite_embedded_nul },
};

test_suite fkstrwrite_suite = { fkstrwrite_tests, sizeof(fkstrwrite_tests) / sizeof(fkstrwrite_tests[0]) };
