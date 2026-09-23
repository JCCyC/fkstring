# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`fkstring` is a small C library implementing a length-tracked, growable
string type (`fkstring`), inspired by Joel Spolsky's "Back to Basics"
article. It's a from-scratch C string buffer with explicit length/capacity
tracking instead of relying on `strlen()`/NUL scanning for every operation.
LGPL-2.1 licensed.

## Commands

- Build everything (`libfkstring.a`, `libfkstring.so`, `smoketest`): `make`
- Clean build artifacts: `make clean`
- Install system-wide: `make install` — requires root (the Makefile runs
  `install -o root -g root`); installs to `$(PREFIX)/lib` and
  `$(PREFIX)/include`, where `PREFIX` defaults to `/usr/local` (edit the
  `Makefile` to change it).
- Compile a single translation unit directly, e.g.: `gcc -I. -Wall -fpic -c fkstring.c -o fkstring.o`
- Run the automated test suite: `make check` (alias: `make test`) — builds
  `libfkstring.a`, then builds and runs `tests/alltests`. Exits nonzero if
  any test fails, so `make check` itself reports an error in that case.

`tests/alltests` is a from-scratch assertion-based suite (no external test
framework) living in `tests/`: one `test_<fn>.c` file per `fkstring.c`/
`fkstdio.c`/`fkstrerr.c` function (or closely related group of functions,
e.g. `test_fkstrcat.c` covers `fkstrcat`/`fkstrcatc`/`fkstrcatone`), each
exposing a `test_suite` of `{ description, test_fn }` cases built with the
`CHECK()` macro from `tests/framework.h`. `tests/alltests.c` collects all
suites and `tests/framework.c`'s `run_suites()` prints `#N (description)...`
(unbuffered, so the cursor visibly sits there during a slow test) followed
by `PASS` or `FAIL - <reason>`. It deliberately does **not** test
`smoketest.c`, which remains the manual demo program described below.

`fksprintf("")` and `fkstrread()` at EOF/with `count==0` are also asserted
against the `fkstring.h` len==0 invariant (`alloc == 0` whenever `len == 0`)
— both code paths originally left a stale nonzero `alloc` behind despite
freeing/NULLing `cstr`, which `fksprintf()`'s and `fkstrread()`'s
`bytesread == 0` handling now correct explicitly. `tests/test_fkpanic.c`
tests `fkpanic()` (which calls `exit(253)`) by forking and inspecting the
child's exit status/stderr, since calling it in-process would kill the
whole test run.

`smoketest.c` (built via `make`, not `make check`) is a separate manual
smoke-test/demo program: it runs a sequence of operations and prints the
resulting `len`/`alloc`/contents after each one via the local `fkshow()`
helper. To eyeball a change interactively, build and run `./smoketest` and
visually diff the output against a known-good run (e.g. `git stash` the
change, capture output, restore, compare).

## Architecture

File layout (small enough to read in full, but the cross-file conventions
below are easy to miss):

- `fkstring.h` — public API: the `fkstring` struct, the `fkstrlen`/
  `fkstrsize`/`fkcstr` accessor macros, and all public function prototypes.
- `fkstring_internal.h` — growth-strategy tunables (`_bumpfactor`,
  `_deflatefactor`, `_minalloc`, `_sprintftry`), error codes (`FKSTRERR_*`),
  and internal helpers (`fkpanic`, `allocforlen`). **This header is also
  installed to `$(PREFIX)/include` by `make install`** alongside
  `fkstring.h` — it isn't private to the build.
- `fkstring.c` — core operations: construction (`fkstrnew`, `fkstrnewb`),
  destruction, duplication, concatenation (`fkstrcat`/`fkstrcatc`/
  `fkstrcatone`, all funneling through the static `fkstrcat_internal`),
  truncation, `fksubstr`, `fkremove`, trimming (`fkltrim`/`fkrtrim`/
  `fktrim`, which funnel through `fkremove`/`fkstrtrunc` rather than
  duplicating the shift/truncate logic), and `fksplit`/`fkarraydestroy`
  (the only functions dealing in `fkstring **` arrays — `fksplit` builds
  each part via `fksubstr` and NULL-terminates the array; `fkarraydestroy`
  is its matching destructor, walking to that NULL terminator), and
  comparison (`fkstrcmp`/`fkstrcasecmp`, both funneling through the static
  `fkstrcmp_internal`, plus `fkstreq`). Comparison conventions: results are
  normalized to -1/0/1, `NULL` sorts before any `fkstring` (two `NULL`s are
  equal), and case folding is ASCII-only (`fkfoldcase`), deliberately not
  locale-dependent `tolower()`, for the same reason the trims use
  `isfkspace` instead of `isspace()`.
- `fkstdio.c` — I/O-adjacent constructors: `fksprintf`, `fkstrwrite`,
  `fkstrread`.
- `fkstrerr.c` — the `errmsgs[]` string table indexed by `FKSTRERR_*`.

Cross-cutting conventions a change should preserve:

1. **Growth/shrink hysteresis.** Appends over-allocate via `allocforlen()`
   using `_bumpfactor` (143%); `fkstrtrunc()` only `realloc()`s back down
   once `alloc` exceeds `newlen * _deflatefactor / 100` (350%) *and*
   `alloc > _minalloc` (16 bytes). This two-threshold scheme is what keeps
   repeated grow/shrink cycles (e.g. in a loop) from thrashing `realloc()`
   on every call. Both constants live in `fkstring_internal.h`; the logic
   lives in `fkstring.c`.
2. **NUL-terminated, but length-authoritative.** Every non-empty `fkstring`
   keeps `cstr` NUL-terminated at `cstr[len]` for easy interop with libc/
   POSIX calls, but `len` (not the terminator) is authoritative — `fkstrnewb()`
   and friends allow embedded NUL bytes within `len`. The invariant noted in
   `fkstring.h`: if `len == 0`, then `alloc == 0` and `cstr == NULL`. Any new
   constructor/mutator needs to keep both of these intact.
3. **OOM handling is non-recoverable by design.** There's no error-return
   path for allocation failure: `fkpanic()` (`fkstring.c`) writes a message
   from `errmsgs[]` to stderr and calls `exit(253)` unconditionally whenever
   `malloc`/`realloc` fails. `NULL` returns from API functions are reserved
   for invalid-argument cases (e.g. a `NULL` `fkstring *`), not OOM. New
   allocating code should follow the same `fkpanic()`-on-OOM pattern rather
   than introducing a different error-handling style.
4. **`fksprintf`'s two-pass `vsnprintf`.** `make_message()` in `fkstdio.c`
   first tries with a guessed buffer size (`_sprintftry`); if `vsnprintf()`
   reports it needed more space, it retries once with the exact size. This
   handles both glibc >= 2.1 (returns the required size) and the older
   glibc 2.0 behavior (returns -1, handled by a 10x size guess) — see the
   `TODO` comment for the known limitation on very old glibc.
5. **`size_t`/`ssize_t` convention.** `fkstring.len`/`.alloc` and all
   length/offset parameters are `size_t`. Only functions mirroring
   `read()`/`write()` semantics (`fkstrwrite`) return `ssize_t`; everything
   else that can legitimately return a byte count uses `size_t` (e.g.
   `fkremove`). Because these are unsigned, bounds must be checked *before*
   subtracting (see how `fkstrcat_internal`, `fkremove`, and `fksubstr`
   compare against `fstr->len` before computing a difference) to avoid
   unsigned underflow/wraparound.

## Future features

A backlog of proposed additions, numbered so a session can be asked to "do
Future feature #N". When one is implemented, remove its entry here (and
renumber nothing — gaps are fine), add tests per the one-`test_<fn>.c`-per-
function convention, and document it in `README.md`. Items are ordered
roughly by usefulness; the top ones (#2, #3 — #1, comparison, is done)
remove the most common reasons users currently fall back to NUL-scanning
libc calls on `fkcstr()`.

**Cross-cutting concerns for every item below:**

- *Overflow in length arithmetic.* `dst->len + src->len` and
  `allocforlen()`'s `* _bumpfactor / 100` can wrap for huge inputs. Consider
  a checked-add helper that `fkpanic()`s on overflow (same non-recoverable
  policy as OOM), and use it in new code.
- *Return-type consistency.* Existing mutators return either `fkstring *`
  (`fkstrcat`, `fkstrtrunc`) or `size_t` (`fkremove`, trims). New functions
  should follow one documented rule (e.g. "returns `dst` for chaining" vs.
  "returns bytes affected") — decide and state it when adding the first one.
- *NUL safety.* Everything must honor `len` and tolerate `cstr == NULL` for
  empty strings: use `memcmp`/`memchr`/`memmem`, never `strcmp`/`strchr`/
  `strstr` on `cstr`.

### Tier 1 — basic gaps

2. **Search: `fkstrfind(hay, needle, start)`, `fkstrfindc(hay, cstr,
   start)`, `fkstrchr(fks, c, start)`, `fkstrrchr(fks, c)`, plus
   `fkstartswith`/`fkendswith`.** Return a `size_t` offset, with a sentinel
   `#define FKSTR_NPOS ((size_t)-1)` for not-found. Use `memchr`/`memmem`;
   `memmem` is a GNU/BSD extension, so feature-test it or provide a fallback.
3. **Formatted append: `fkstrcatf(dst, fmt, ...)`, plus `fkvsprintf` and
   `fkstrcatvf` (`va_list`) variants.** Refactor `make_message()`'s
   two-pass `vsnprintf` to write directly at `dst->cstr + dst->len` after an
   `allocforlen()` grow; `fksprintf` then becomes `fkstrcatf` on an empty
   string. The `va_list` variants let users build their own wrappers (e.g.
   loggers). Keep the `format(printf, ...)` attribute.
4. **Join: `fkjoin(fkstring **arr, const char *sep)`.** Inverse of
   `fksplit`, over the same NULL-terminated array. Pre-compute the total
   length so it allocates exactly once.

### Tier 2 — editing primitives

5. **Insert: `fkinsert(fks, pos, src)` / `fkinsertc(fks, pos, cstr)`.**
   Complements `fkremove`. Bounds-check `pos` before any subtraction.
6. **Replace: `fkreplace(fks, old, new, max_count)`.** Build on #2 plus #5/
   `fkremove`, but prefer a two-pass count-then-build so it grows once
   instead of per match. Depends on #2.
7. **Capacity control: `fkreserve(fks, n)`, `fkshrinktofit(fks)`.**
   Design tension to resolve first: `fkreserve` on an empty string conflicts
   with the `len == 0 ⇒ alloc == 0` invariant. Options: relax the invariant
   (`len == 0 ⇒ cstr == NULL` *or* `cstr[0] == '\0'`), or make reserve a
   no-op on empty strings (much less useful). Also, a reservation would be
   undone by the next `fkstrtrunc()` under the 350% `_deflatefactor`
   threshold — likely needs a per-string "don't shrink below N" field.
   Discuss with the user before implementing; it touches the struct.
8. **Character-set trims: `fkltrimset(fks, const char *set)`,
   `fkrtrimset`, `fktrimset`.** Generalize the `\s`-only trims; the existing
   `fkltrim`/`fkrtrim`/`fktrim` can become thin wrappers.
9. **Richer splitting: `fksplitstr(src, const fkstring *delim)`,
   `fksplitany(src, const char *delims)`.** Consider a `max_parts` argument
   and/or a flag to collapse empty fields (`"a,,b"` vs. `"a  b"` need
   different behavior). Reuse `fkarraydestroy` as the destructor.

### Tier 3 — I/O conveniences

10. **Line reading: `fkreadline(FILE *fp)` / `fkreadline_fd(int fd)`.**
    Arbitrary-length line reads. The `FILE *` version can use
    `getc_unlocked`; the fd version needs a buffering strategy (one byte per
    `read()` is slow) — decide where leftover bytes live.
11. **Whole-file reads: `fkslurp(int fd)` / `fkslurpfile(const char
    *path)`.** Use `fstat` as a size hint for regular files; fall back to
    `_bumpfactor` growth for pipes/sockets.
12. **`fkfwrite(FILE *fp, const fkstring *fks)`.** stdio counterpart to
    `fkstrwrite`; preserves embedded NULs (unlike `fputs(fkcstr(...))`).

### Tier 4 — nice to have

13. **Case conversion: `fktoupper(fks)` / `fktolower(fks)`.** In place, via
    `<ctype.h>`. Document that it is byte-wise, not UTF-8 aware.
14. **Hashing: `fkstrhash(const fkstring *fks)`.** FNV-1a (or similar) over
    `len` bytes, for users building hash tables keyed by `fkstring`.
15. **Non-owning views: `FKSTR_LIT("abc")` / `fkstrview(ptr, len)`.**
    Produce a stack `fkstring` usable as a `const fkstring *` argument
    without heap allocation (e.g. `fkstrcat(dst, &FKSTR_LIT(", "))`). Must
    define and ideally enforce that views never reach mutators or
    `fkstrdestroy` — e.g. an `alloc == 0 && len > 0` "borrowed" marker.
16. **Escaping: `fkescape(fks)` / `fkunescape(fks)`.** C-style escaping of
    non-printables and embedded NULs; useful for debugging, and could
    simplify `smoketest.c`'s `fkshow()`.
17. **Invariant checking: `FKSTR_DEBUG` build flag with
    `fkstrcheck(const fkstring *fks)`.** Asserts `cstr[len] == '\0'`,
    `len < alloc`, and the `len == 0` rule. Wire it into the test suite
    after every mutating operation.
