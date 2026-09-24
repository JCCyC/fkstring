#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fkstring.h>
#include "framework.h"

static int test_fkreadline_lines_keep_newline(char *errbuf, size_t errbuflen)
{
	char		buf[] = "one\ntwo\n";
	FILE		*fp;
	fkstring	*s;

	CHECK((fp = fmemopen(buf, strlen(buf), "r")) != NULL, "fmemopen() failed");

	s = fkreadline(fp);
	CHECK(s != NULL, "first fkreadline() returned NULL");
	CHECK(fkstrlen(s) == 4, "expected len 4, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "one\n") == 0, "expected 'one\\n', got '%s'", fkcstr(s));
	fkstrdestroy(s);

	s = fkreadline(fp);
	CHECK(s != NULL, "second fkreadline() returned NULL");
	CHECK(strcmp(fkcstr(s), "two\n") == 0, "expected 'two\\n', got '%s'", fkcstr(s));
	fkstrdestroy(s);

	s = fkreadline(fp);
	CHECK(s == NULL, "expected NULL at EOF, got '%s'", fkcstr(s));
	fclose(fp);
	return 1;
}

static int test_fkreadline_last_line_without_newline(char *errbuf, size_t errbuflen)
{
	char		buf[] = "a\nlast";
	FILE		*fp;
	fkstring	*s;

	CHECK((fp = fmemopen(buf, strlen(buf), "r")) != NULL, "fmemopen() failed");
	fkstrdestroy(fkreadline(fp));

	s = fkreadline(fp);
	CHECK(s != NULL, "fkreadline() returned NULL for an unterminated last line");
	CHECK(fkstrlen(s) == 4, "expected len 4, got %zu", fkstrlen(s));
	CHECK(strcmp(fkcstr(s), "last") == 0, "expected 'last', got '%s'", fkcstr(s));
	fkstrdestroy(s);

	CHECK(fkreadline(fp) == NULL, "expected NULL at EOF");
	fclose(fp);
	return 1;
}

static int test_fkreadline_empty_line(char *errbuf, size_t errbuflen)
{
	char		buf[] = "\nx";
	FILE		*fp;
	fkstring	*s;

	CHECK((fp = fmemopen(buf, strlen(buf), "r")) != NULL, "fmemopen() failed");
	s = fkreadline(fp);
	CHECK(s != NULL, "fkreadline() returned NULL for an empty line");
	CHECK(fkstrlen(s) == 1, "expected len 1, got %zu", fkstrlen(s));
	CHECK(fkcstr(s)[0] == '\n' && fkcstr(s)[1] == '\0', "expected '\\n'");
	fkstrdestroy(s);
	fclose(fp);
	return 1;
}

static int test_fkreadline_empty_stream_returns_null(char *errbuf, size_t errbuflen)
{
	int		fds[2];
	FILE		*fp;

	/* fmemopen() of size 0 is not portable; an empty pipe hits EOF at once. */
	CHECK(pipe(fds) == 0, "pipe() failed");
	close(fds[1]);
	CHECK((fp = fdopen(fds[0], "r")) != NULL, "fdopen() failed");
	CHECK(fkreadline(fp) == NULL, "expected NULL on an empty stream");
	CHECK(!ferror(fp), "expected no stream error at plain EOF");
	fclose(fp);
	return 1;
}

static int test_fkreadline_embedded_nul(char *errbuf, size_t errbuflen)
{
	char		buf[] = { 'a', '\0', 'b', '\n', 'c' };
	FILE		*fp;
	fkstring	*s;

	CHECK((fp = fmemopen(buf, sizeof(buf), "r")) != NULL, "fmemopen() failed");
	s = fkreadline(fp);
	CHECK(s != NULL, "fkreadline() returned NULL");
	CHECK(fkstrlen(s) == 4, "expected len 4 (NUL kept), got %zu", fkstrlen(s));
	CHECK(memcmp(fkcstr(s), "a\0b\n", 5) == 0, "expected 'a\\0b\\n' plus terminator");
	fkstrdestroy(s);
	fclose(fp);
	return 1;
}

static int test_fkreadline_long_line(char *errbuf, size_t errbuflen)
{
	enum { LONG = 100000 };
	static char	buf[LONG + 1];
	FILE		*fp;
	fkstring	*s;
	size_t		i;

	/* Many growth steps past _minalloc; checks every byte lands in order. */
	for (i = 0; i < LONG; i++)
		buf[i] = 'a' + i % 26;
	buf[LONG] = '\n';

	CHECK((fp = fmemopen(buf, sizeof(buf), "r")) != NULL, "fmemopen() failed");
	s = fkreadline(fp);
	CHECK(s != NULL, "fkreadline() returned NULL");
	CHECK(fkstrlen(s) == LONG + 1, "expected len %d, got %zu", LONG + 1, fkstrlen(s));
	CHECK(fkstrsize(s) > fkstrlen(s), "expected alloc > len, got alloc %zu", fkstrsize(s));
	CHECK(memcmp(fkcstr(s), buf, LONG + 1) == 0, "line contents differ from input");
	CHECK(fkcstr(s)[LONG + 1] == '\0', "expected NUL terminator at cstr[len]");
	fkstrdestroy(s);
	CHECK(fkreadline(fp) == NULL, "expected NULL at EOF");
	fclose(fp);
	return 1;
}

static int test_fkreadline_null_fp_returns_null(char *errbuf, size_t errbuflen)
{
	CHECK(fkreadline(NULL) == NULL, "expected NULL for a NULL fp");
	return 1;
}

static test_case fkreadline_tests[] = {
	{ "fkreadline() returns successive lines with their '\\n', then NULL", test_fkreadline_lines_keep_newline },
	{ "fkreadline() returns an unterminated last line without a '\\n'", test_fkreadline_last_line_without_newline },
	{ "fkreadline() returns an empty line as a 1-byte \"\\n\"", test_fkreadline_empty_line },
	{ "fkreadline() on an empty stream returns NULL", test_fkreadline_empty_stream_returns_null },
	{ "fkreadline() preserves embedded NULs", test_fkreadline_embedded_nul },
	{ "fkreadline() reads a 100000-byte line intact", test_fkreadline_long_line },
	{ "fkreadline() with a NULL fp returns NULL", test_fkreadline_null_fp_returns_null },
};

test_suite fkreadline_suite = { fkreadline_tests, sizeof(fkreadline_tests) / sizeof(fkreadline_tests[0]) };
