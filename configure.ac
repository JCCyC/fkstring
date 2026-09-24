AC_PREREQ([2.69])
# Keep in sync with FKSTRING_VERSION* in fkstring.h (make check enforces it).
AC_INIT([fkstring], [0.9.1])
AC_CONFIG_SRCDIR([fkstring.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])

AC_PROG_CC
AM_PROG_AR
LT_INIT

# memmem() is a GNU/BSD extension, only standardized in POSIX.1-2024.
# fkstring.c guesses from platform macros when FKSTR_HAVE_MEMMEM is unset;
# here we test for it instead. To build the memchr()+memcmp() fallback,
# configure with ac_cv_func_memmem=no.
AC_CHECK_FUNC([memmem], [fk_have_memmem=1], [fk_have_memmem=0])
AC_DEFINE_UNQUOTED([FKSTR_HAVE_MEMMEM], [$fk_have_memmem],
                   [Define to 1 if memmem() is available, 0 otherwise.])

AC_PATH_PROG([VALGRIND], [valgrind])
# For make release. shasum (Perl, on BSD/macOS) prints the same format.
AC_CHECK_PROGS([SHA256SUM], [sha256sum shasum])
AS_IF([test "x$SHA256SUM" = xshasum], [SHA256SUM="shasum -a 256"])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
