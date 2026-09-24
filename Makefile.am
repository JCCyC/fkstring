ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -Wall

lib_LTLIBRARIES = libfkstring.la
libfkstring_la_SOURCES = fkstring.c fkstdio.c fkstrerr.c
# Libtool's current:revision:age, independent of the package version. On
# each release: implementation-only changes bump revision; added interfaces
# bump current and age and zero revision; removed or changed interfaces bump
# current and zero revision and age.
libfkstring_la_LDFLAGS = -version-info 0:0:0

# fkstring_internal.h is part of the installed API, not private to the build.
include_HEADERS = fkstring.h fkstring_internal.h

EXTRA_DIST = LICENSE NEWS README.md

# One page per function group plus the fkstring(3) overview; every other
# public name is a one-line .so stub pointing at its group's page.
dist_man3_MANS = \
	man/fkalloc.3 \
	man/fkarraydestroy.3 \
	man/fkcalloc.3 \
	man/fkcat.3 \
	man/fkcatf.3 \
	man/fkcatfd.3 \
	man/fkcstr.3 \
	man/fkendswith.3 \
	man/fkendswithc.3 \
	man/fkfit.3 \
	man/fkinsert.3 \
	man/fkinsertc.3 \
	man/fkjoin.3 \
	man/fkltrim.3 \
	man/fkreadline.3 \
	man/fkremove.3 \
	man/fkreplace.3 \
	man/fkreplacec.3 \
	man/fkrtrim.3 \
	man/fkslack.3 \
	man/fkslurp.3 \
	man/fkslurpfile.3 \
	man/fksplit.3 \
	man/fksprintf.3 \
	man/fkstartswith.3 \
	man/fkstartswithc.3 \
	man/fkstrcasecmp.3 \
	man/fkstrcat.3 \
	man/fkstrcatc.3 \
	man/fkstrcatf.3 \
	man/fkstrcatone.3 \
	man/fkstrcatvf.3 \
	man/fkstrchr.3 \
	man/fkstrcmp.3 \
	man/fkstrdestroy.3 \
	man/fkstrdup.3 \
	man/fkstreq.3 \
	man/fkstrfind.3 \
	man/fkstrfindc.3 \
	man/fkstring.3 \
	man/fkstrlen.3 \
	man/fkstrnew.3 \
	man/fkstrnewb.3 \
	man/fkstrrchr.3 \
	man/fkstrread.3 \
	man/fkstrsize.3 \
	man/fkstrtrunc.3 \
	man/fkstrversion.3 \
	man/fkstrwrite.3 \
	man/fksubstr.3 \
	man/fktrim.3 \
	man/fkvsprintf.3

check_PROGRAMS = tests/alltests
dist_check_SCRIPTS = tests/manpages.sh
TESTS = tests/alltests tests/manpages.sh

tests_alltests_SOURCES = \
	tests/framework.c \
	tests/framework.h \
	tests/test_allocforlen.c \
	tests/test_fkaddlen.c \
	tests/test_fkmullen.c \
	tests/test_fkpanic.c \
	tests/test_fkstrerr.c \
	tests/test_fkstrversion.c \
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
# PACKAGE_VERSION reaches this test through DEFS, which object files don't
# otherwise depend on; recompile it whenever configure regenerates Makefile.
tests/test_fkstrversion.$(OBJEXT): Makefile
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

# Maintainer target: distcheck, then write a sha256sum-style .sha256 next
# to each archive for uploading with it (verify with sha256sum -c).
release: distcheck
	@if test -z "$(SHA256SUM)"; then \
	  echo "sha256sum/shasum not found; install one and rerun configure" >&2; \
	  exit 1; \
	fi
	for f in $(DIST_ARCHIVES); do $(SHA256SUM) "$$f" > "$$f.sha256" || exit 1; done

.PHONY: test memcheck release
