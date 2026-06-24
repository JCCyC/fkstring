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
  is its matching destructor, walking to that NULL terminator).
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
