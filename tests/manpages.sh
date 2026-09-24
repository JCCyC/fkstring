#!/bin/sh
# Checks the man pages against fkstring.h and Makefile.am: every public
# function and macro has a page (full or .so stub), every stub points at an
# existing page, every page is installed via dist_man3_MANS, and, if groff is
# available, every full page lints clean.

srcdir=${srcdir:-$(dirname "$0")/..}
man=$srcdir/man
fail=0

err()
{
	echo "FAIL - $*"
	fail=1
}

names=$(sed -n -e 's/^[a-z_][a-z_]* [ *]*\(fk[a-z]*\)(.*/\1/p' \
	       -e 's/^#define \(fk[a-z]*\)(.*/\1/p' "$srcdir/fkstring.h")
[ -n "$names" ] || err "no function names found in fkstring.h"

for n in $names; do
	[ -f "$man/$n.3" ] || err "$n has no man page ($man/$n.3)"
done

for f in "$man"/*.3; do
	b=$(basename "$f")
	grep -q "man/$b\\b" "$srcdir/Makefile.am" || err "$b is not listed in Makefile.am"

	so=$(sed -n 's/^\.so man3\/\(.*\)/\1/p' "$f")
	if [ -n "$so" ]; then
		[ -f "$man/$so" ] || err "$b: .so target $so does not exist"
		sed -n '/^\.SH NAME/{n;p;}' "$man/$so" | grep -q "\\b${b%.3}\\b" ||
			err "$b: not named in the NAME section of $so"
		continue
	fi

	if command -v groff >/dev/null 2>&1; then
		out=$(LC_ALL=C.UTF-8 groff -man -ww -z -Tutf8 "$f" 2>&1)
		[ -z "$out" ] || err "$b: groff warnings: $out"
	fi
done

[ $fail -eq 0 ] && echo "PASS - man pages"
exit $fail
