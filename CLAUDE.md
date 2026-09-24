# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`fkstring` is a small C library implementing a length-tracked, growable
string type (`fkstring`), inspired by Joel Spolsky's "Back to Basics"
article. It's a from-scratch C string buffer with explicit length/capacity
tracking instead of relying on `strlen()`/NUL scanning for every operation.
LGPL-2.1 licensed.

## Commands

- Bootstrap (git checkout only, after editing `configure.ac`/`Makefile.am`
  or on a fresh clone): `autoreconf -fi`, then `./configure`. Autotools with a
  flat layout: one `configure.ac` and one top-level `Makefile.am` (no
  `SUBDIRS`; `tests/` sources are listed there via `subdir-objects`). All
  generated files (`configure`, `Makefile.in`, `build-aux/`, `m4/*`, ...)
  are gitignored.
- Build everything (`libfkstring.la`, i.e. `.libs/libfkstring.{a,so}`):
  `make`
- Clean build artifacts: `make clean` (`make distclean` also removes
  `configure`'s output).
- Install: `make install` to `$prefix/lib` and `$prefix/include`
  (`--prefix`, default `/usr/local`).
- Compile a single translation unit directly, e.g.: `gcc -I. -Wall -fpic -c fkstring.c -o fkstring.o`
- Run the automated test suite: `make check` (alias: `make test`) — builds
  and runs `tests/alltests` through Automake's test driver, which writes the
  per-test output to `tests/alltests.log` (run `./tests/alltests` directly
  to see it live). Exits nonzero if any test fails.
- Run the suite under Valgrind: `make memcheck` (custom rule in
  `Makefile.am`; `configure` looks for `valgrind`). `tests/alltests` is
  linked with libtool's `-static`, which links `libfkstring` statically but
  libc dynamically, so the same binary serves both targets: it needs no
  libtool wrapper script and Valgrind can still intercept `malloc()`. Exits
  nonzero on any test failure, leak, or memory error.
- Man pages: `man/*.3`, English only for now, in `man(7)` macros (not
  `mdoc`), installed via `dist_man3_MANS` in `Makefile.am`. There's one full
  page per function group, following the `test_<fn>.c` grouping, plus the
  `fkstring(3)` overview (type, invariants, tunables, `DIAGNOSTICS`). Every
  other public name is a one-line `.so man3/<page>.3` stub. Preview with
  `man -l man/<page>.3`. `make check` also runs `tests/manpages.sh`, which
  fails if a function or macro in `fkstring.h` has no page, a page is
  missing from `dist_man3_MANS`, a stub's target doesn't list its name
  under `NAME`, or `groff -man -ww` warns. Bump the `.TH` date on any page
  you edit.
- Versioning: the package version (SemVer) lives in two places that must
  agree: `AC_INIT` in `configure.ac` and the `FKSTRING_VERSION*` macros at
  the top of `fkstring.h` (`fkstrversion()` in `fkstring.c` returns
  `FKSTRING_VERSION`). `tests/test_fkstrversion.c` fails `make check` if
  they drift. `NEWS` is the changelog (user-facing, per release; there is
  no `ChangeLog`): after every user-visible change, add a line under its
  `Unreleased` heading right away, not at release time. Bumping the version
  means editing both places, `autoreconf -fi`, renaming `Unreleased` to
  `fkstring X.Y.Z (date)` with a fresh empty `Unreleased` above it, and
  tagging `vX.Y.Z`. The libtool `-version-info` in
  `Makefile.am` is independent (see the comment there): update it per
  libtool's rules at each release, not per package bump.
- Release tarball: `make dist`; `make distcheck` also verifies a VPATH
  build, `make check` and install/uninstall from it. `make release` (custom
  rule in `Makefile.am`) runs `distcheck`, then writes a `.sha256` next to
  each archive in `$(DIST_ARCHIVES)`; `configure` picks `sha256sum` or
  `shasum -a 256` as `SHA256SUM`. Build releases from a clean checkout of
  the tag, since `make dist` packs uncommitted edits to distributed files,
  and upload the tarball and its `.sha256` to a GitHub release (a pushed
  tag alone only gets `git archive` snapshots, which lack `configure`).

`tests/alltests` is a from-scratch assertion-based suite (no external test
framework) living in `tests/`: one `test_<fn>.c` file per `fkstring.c`/
`fkstdio.c`/`fkstrerr.c` function (or closely related group of functions,
e.g. `test_fkstrcat.c` covers `fkstrcat`/`fkstrcatc`/`fkstrcatone`), each
exposing a `test_suite` of `{ description, test_fn }` cases built with the
`CHECK()` macro from `tests/framework.h`. `tests/alltests.c` collects all
suites and `tests/framework.c`'s `run_suites()` prints `#N (description)...`
(unbuffered, so the cursor visibly sits there during a slow test) followed
by `PASS` or `FAIL - <reason>`.

`fksprintf("")` (and `fkstrcatf()` with empty output onto an empty string)
and `fkstrread()` at EOF/with `count==0` are also asserted against the
`fkstring.h` len==0 invariant (`alloc == 0` whenever `len == 0`). Both code
paths originally left a stale nonzero `alloc` behind despite freeing/NULLing
`cstr`. `fkstrcatvf()`'s `dst->len == 0` tail and `fkstrread()`'s
`bytesread == 0` handling now correct this explicitly. `tests/test_fkpanic.c`
tests `fkpanic()` (which calls `exit(253)`) by forking and inspecting the
child's exit status/stderr, since calling it in-process would kill the
whole test run.

## Architecture

File layout (small enough to read in full, but the cross-file conventions
below are easy to miss):

- `fkstring.h` — public API: the `fkstring` struct, the `fkstrlen`/
  `fkstrsize`/`fkcstr` accessor macros, and all public function prototypes.
- `fkstring_internal.h` — growth-strategy tunables (`_bumpfactor`,
  `_deflatefactor`, `_minalloc`, `_sprintftry`, `_slurptry`,
  `_catbufsize`), error codes (`FKSTRERR_*`),
  and internal helpers (`fkpanic`, `allocforlen`), both `static inline`
  so they get inlined despite `-fpic`. **This header is also
  installed to `$prefix/include` by `make install`** alongside
  `fkstring.h` — it isn't private to the build.
- `fkstring.c` — core operations: construction (`fkstrnew`, `fkstrnewb`,
  and `fkalloc`/`fkcalloc`, via the static `fkalloc_internal`),
  destruction, duplication, concatenation and insertion (`fkstrcat`/
  `fkstrcatc`/`fkstrcatone` and `fkinsert`/`fkinsertc`, all funneling
  through the static `fkinsert_internal`, with appending being an insert at
  `pos == len`; it copies `src` aside first when it points into `dst`'s own
  buffer, since growth can move that buffer), replacement (`fkreplace`/
  `fkreplacec`, via the static `fkreplace_internal`, which counts matches
  first, then builds the result in one fresh buffer), truncation, capacity
  control (`fkslack`/`fkfit`, see convention 1), `fksubstr`, `fkremove`, trimming (`fkltrim`/`fkrtrim`/
  `fktrim`, which funnel through `fkremove`/`fkstrtrunc` rather than
  duplicating the shift/truncate logic), and `fksplit`/`fkjoin`/
  `fkarraydestroy` (the only functions dealing in `fkstring **` arrays —
  `fksplit` builds each part via `fksubstr` and NULL-terminates the array;
  `fkjoin` is its inverse, summing lengths via `fkaddlen` first so it
  allocates once; `fkarraydestroy` is the array's destructor, walking to
  that NULL terminator),
  comparison (`fkstrcmp`/`fkstrcasecmp`, both funneling through the static
  `fkstrcmp_internal`, plus `fkstreq`), and case conversion (`fktoupper`/
  `fktolower`, via the static `fkconvcase_internal`, in place and never
  reallocating). Comparison conventions: results are
  normalized to -1/0/1, `NULL` sorts before any `fkstring` (two `NULL`s are
  equal), and case folding is ASCII-only (`fkfoldcase`), deliberately not
  locale-dependent `tolower()`, for the same reason the trims use
  `isfkspace` instead of `isspace()`. Case conversion uses the same
  ASCII-only rule, so `fktolower` agrees with `fkstrcasecmp`.
  Search (`fkstrfind`/`fkstrfindc`, both funneling through the static
  `fkstrfind_internal`, plus `fkstrchr`/`fkstrrchr`, and `fkstartswith`/
  `fkstartswithc` and `fkendswith`/`fkendswithc`, each pair funneling
  through a static `*_internal` helper): offsets are `size_t`, with
  `FKSTR_NPOS` (`fkstring.h`) for not-found or `NULL` arguments, and an
  empty needle matches at `start` iff `start <= len`. Substring search goes
  through the static `fkmemmem`, which calls `memmem()` when
  `FKSTR_HAVE_MEMMEM` (auto-detected at the top of `fkstring.c`, which also
  `#define`s `_GNU_SOURCE`) is 1, and otherwise uses a `memchr`+`memcmp`
  fallback. Under autotools, `configure` tests for `memmem()` and passes
  `-DFKSTR_HAVE_MEMMEM=0/1` itself, so the in-source guess is only a
  fallback for non-autotools builds. `make check` only exercises the
  `memmem()` path on glibc; to test the fallback, run
  `./configure ac_cv_func_memmem=no && make clean check`.
- `fkstdio.c` — formatting and I/O: `fkstrcatvf` (the formatting core),
  its wrappers `fkstrcatf`/`fkvsprintf`/`fksprintf`, plus `fkstrwrite`,
  `fkstrread`, and `fkreadline` (one line from a `FILE *`, via
  `getc_unlocked()` under a single `flockfile()`; keeps the `'\n'`, returns
  `NULL` at EOF-with-nothing-read, unlike `fkstrread`'s empty `fkstring`),
  and `fkslurp`/`fkslurpfile` (fd to EOF in one `read()` loop, retrying
  `EINTR`; for regular files `fstat()`'s size minus the current offset,
  plus 2 so the EOF-confirming `read()` needs no growth, sizes the first
  buffer. It's only a hint: `/proc` files report 0. Otherwise it starts
  from `_slurptry` and grows via `allocforlen()`. Leftover slack is
  `fkfit()`ted past the `fkstrtrunc()` deflate threshold), and
  `fkcatfd`/`fkcatf` (plus the `fkcat(path)` macro for fd 1, in
  `fkstring.h`), both via the static `fkcat_internal`, which streams a
  file to an fd or `FILE *` through one `_catbufsize` buffer without
  building an `fkstring`. They return `ssize_t` bytes copied (saturating at
  `SSIZE_MAX`), or -1 with `errno` (`EINVAL` for `NULL` arguments).
  `fkstring.h` includes `<stdio.h>` for `fkreadline`'s `FILE *`.
- `fkstrerr.c` — the `errmsgs[]` string table indexed by `FKSTRERR_*`.

Cross-cutting conventions a change should preserve:

1. **Growth/shrink hysteresis.** Appends over-allocate via `allocforlen()`
   using `_bumpfactor` (143%); `fkstrtrunc()` only `realloc()`s back down
   once `alloc` exceeds `newlen * _deflatefactor / 100` (350%) *and*
   `alloc > _minalloc` (16 bytes). This two-threshold scheme is what keeps
   repeated grow/shrink cycles (e.g. in a loop) from thrashing `realloc()`
   on every call. Both constants live in `fkstring_internal.h`; the logic
   lives in `fkstring.c`. `fkslack(fks, n)` and `fkfit(fks)` deliberately
   bypass it: they set `alloc` to exactly `len + n + 1` (grow only) and
   `len + 1`, ignoring `_bumpfactor`/`_minalloc`. There is no per-string
   floor, so a later `fkstrtrunc()` (and thus `fkremove()` or a trim) past
   the `_deflatefactor` threshold undoes an `fkslack()`. The user accepted
   this rather than adding a struct field. Both are no-ops on empty strings,
   preserving the `len == 0` invariant; `fkalloc()`/`fkcalloc()` cover
   preallocating those.
2. **NUL-terminated, but length-authoritative.** Every non-empty `fkstring`
   keeps `cstr` NUL-terminated at `cstr[len]` for easy interop with libc/
   POSIX calls, but `len` (not the terminator) is authoritative — `fkstrnewb()`
   and friends allow embedded NUL bytes within `len`. The invariant noted in
   `fkstring.h`: if `len == 0`, then `alloc == 0` and `cstr == NULL`. Any new
   constructor/mutator needs to keep both of these intact.
3. **OOM handling is non-recoverable by design.** There's no error-return
   path for allocation failure: `fkpanic()` (`fkstring_internal.h`) writes a message
   from `errmsgs[]` to stderr and calls `exit(253)` unconditionally whenever
   `malloc`/`realloc` fails. `NULL` returns from API functions are reserved
   for invalid-argument cases (e.g. a `NULL` `fkstring *`), not OOM. New
   allocating code should follow the same `fkpanic()`-on-OOM pattern rather
   than introducing a different error-handling style.
4. **Two-pass `vsnprintf` in `fkstrcatvf()`.** All four formatting
   functions go through `fkstrcatvf()` in `fkstdio.c`. Its first pass
   formats directly at `dst->cstr + dst->len` into the existing slack, on a
   `va_copy()`. An empty `dst` gets a `_sprintftry`-byte buffer for this
   pass. If `vsnprintf()` reports it needed more space, `dst` grows via
   `allocforlen()` and the second pass consumes the caller's `ap`. This
   handles both glibc >= 2.1 (returns the required size) and the older
   glibc 2.0 behavior (returns -1, handled by a 10x size guess) — see the
   `TODO` comment for the known limitation on very old glibc. A real
   `vsnprintf()` failure (`EOVERFLOW` past `INT_MAX`, `EILSEQ` for `%ls`/
   `%lc`) returns `NULL` with `errno` set and `dst`'s contents intact (the
   static `fkstrcatvf_fail`). Since glibc 2.0 also returned -1 on mere
   truncation, a first-pass -1 is only treated as an error when `errno` is
   nonzero (it's cleared first). `FKSTRERR_VSNPRINTF` now panics only if
   the second pass overflows the size the first one reported.
5. **`size_t`/`ssize_t` convention.** `fkstring.len`/`.alloc` and all
   length/offset parameters are `size_t`. Only functions mirroring
   `read()`/`write()` semantics (`fkstrwrite`) return `ssize_t`; everything
   else that can legitimately return a byte count uses `size_t` (e.g.
   `fkremove`). Because these are unsigned, bounds must be checked *before*
   subtracting (see how `fkinsert`, `fkremove`, and `fksubstr`
   compare against `fstr->len` before computing a difference) to avoid
   unsigned underflow/wraparound.

## Future features

A backlog of proposed additions, numbered so a session can be asked to "do
Future feature #N". When one is implemented, remove its entry here (and
renumber nothing — gaps are fine), add tests per the one-`test_<fn>.c`-per-
function convention, and document it in `README.md` and in a man page
(a new group page or a `.so` stub, listed in `dist_man3_MANS`). Items are ordered
roughly by usefulness (#1 comparison, #2 search, #3 formatted append, #4
join, #5 insert, #6 replace, #7 capacity control, #10 line reading,
#11 whole-file reads and #13 case conversion are done; `fkcat*` was added outside the backlog); the top remaining one is #8, character-set trims.

**Cross-cutting concerns for every item below:**

- *Overflow in length arithmetic.* Use `fkaddlen(a, b)`
  (`fkstring_internal.h`) for length sums in new code, and `fkmullen(a, b)`
  for products (as in `fkcalloc`). It `fkpanic()`s with
  `FKSTRERR_OVERFLOW` on wraparound, the same non-recoverable policy as OOM.
  `allocforlen()`'s `len * _bumpfactor` is still unchecked. That
  multiply wraps at `SIZE_MAX / 143`, only about 30 MB on 32-bit.
- *Return-type rule* (settled by `fkstrcatf`). Mutators that append or
  otherwise grow/rewrite `dst` return `dst` for chaining, or `NULL` for
  invalid arguments (`fkstrcat`/`fkstrcatc`/`fkstrcatone`, `fkstrcatf`,
  `fkstrtrunc`, `fkinsert`, `fkreplace`, `fkslack`, `fkfit`, `fktoupper`/
  `fktolower`).
  Mutators that only remove bytes return a `size_t` count or length
  (`fkremove`, trims, #8).
- *NUL safety.* Everything must honor `len` and tolerate `cstr == NULL` for
  empty strings: use `memcmp`/`memchr`/`memmem`, never `strcmp`/`strchr`/
  `strstr` on `cstr`.

### Tier 2 — editing primitives

8. **Character-set trims: `fkltrimset(fks, const char *set)`,
   `fkrtrimset`, `fktrimset`.** Generalize the `\s`-only trims; the existing
   `fkltrim`/`fkrtrim`/`fktrim` can become thin wrappers.
9. **Richer splitting: `fksplitstr(src, const fkstring *delim)`,
   `fksplitany(src, const char *delims)`.** Consider a `max_parts` argument
   and/or a flag to collapse empty fields (`"a,,b"` vs. `"a  b"` need
   different behavior). Reuse `fkarraydestroy` as the destructor.

### Tier 3 — I/O conveniences

12. **`fkfwrite(FILE *fp, const fkstring *fks)`.** stdio counterpart to
    `fkstrwrite`; preserves embedded NULs (unlike `fputs(fkcstr(...))`).

### Tier 4 — nice to have

14. **Hashing: `fkstrhash(const fkstring *fks)`.** FNV-1a (or similar) over
    `len` bytes, for users building hash tables keyed by `fkstring`.
15. **Non-owning views: `FKSTR_LIT("abc")` / `fkstrview(ptr, len)`.**
    Produce a stack `fkstring` usable as a `const fkstring *` argument
    without heap allocation (e.g. `fkstrcat(dst, &FKSTR_LIT(", "))`). Must
    define and ideally enforce that views never reach mutators or
    `fkstrdestroy` — e.g. an `alloc == 0 && len > 0` "borrowed" marker.
16. **Escaping: `fkescape(fks)` / `fkunescape(fks)`.** C-style escaping of
    non-printables and embedded NULs; useful for debugging (e.g. in test
    failure messages).
17. **Invariant checking: `FKSTR_DEBUG` build flag with
    `fkstrcheck(const fkstring *fks)`.** Asserts `cstr[len] == '\0'`,
    `len < alloc`, and the `len == 0` rule. Wire it into the test suite
    after every mutating operation.
