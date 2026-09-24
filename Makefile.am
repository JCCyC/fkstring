ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -Wall

lib_LTLIBRARIES = libfkstring.la
libfkstring_la_SOURCES = fkstring.c fkstdio.c fkstrerr.c
libfkstring_la_LDFLAGS = -version-info 0:0:0

# fkstring_internal.h is part of the installed API, not private to the build.
include_HEADERS = fkstring.h fkstring_internal.h

EXTRA_DIST = LICENSE README.md

check_PROGRAMS = tests/alltests
TESTS = tests/alltests

tests_alltests_SOURCES = \
	tests/framework.c \
	tests/framework.h \
	tests/test_allocforlen.c \
	tests/test_fkaddlen.c \
	tests/test_fkmullen.c \
	tests/test_fkpanic.c \
	tests/test_fkstrerr.c \
	tests/test_fkstrnew.c \
	tests/test_fkalloc.c \
	tests/test_fkstrdestroy.c \
	tests/test_fkstrdup.c \
	tests/test_fkstrtrunc.c \
	tests/test_fkslack.c \
	tests/test_fkstrcat.c \
	tests/test_fkstrcatf.c \
	tests/test_fkinsert.c \
	tests/test_fksubstr.c \
	tests/test_fkremove.c \
	tests/test_fktrim.c \
	tests/test_fksplit.c \
	tests/test_fkjoin.c \
	tests/test_fkstrcmp.c \
	tests/test_fkstrfind.c \
	tests/test_fkstrchr.c \
	tests/test_fkstartswith.c \
	tests/test_fkreplace.c \
	tests/test_fksprintf.c \
	tests/test_fkstrwrite.c \
	tests/test_fkstrread.c \
	tests/test_fkreadline.c \
	tests/test_fkslurp.c \
	tests/test_fkcat.c \
	tests/alltests.c
tests_alltests_LDADD = libfkstring.la
# Libtool's -static links libfkstring statically but libc dynamically, so the
# binary runs in place without a wrapper script and Valgrind can still
# intercept malloc()/free().
tests_alltests_LDFLAGS = -static

test: check

memcheck: tests/alltests$(EXEEXT)
	@if test -z "$(VALGRIND)"; then \
	  echo "valgrind not found; install it and rerun configure" >&2; \
	  exit 1; \
	fi
	$(LIBTOOL) --mode=execute $(VALGRIND) --leak-check=full \
	  --show-leak-kinds=all --error-exitcode=99 tests/alltests$(EXEEXT)

.PHONY: test memcheck
