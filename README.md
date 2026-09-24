# fkstring

F\*\*\*ed Strings — a small C string library, inspired by
["Back to Basics"](https://www.joelonsoftware.com/2001/12/11/back-to-basics/).

`fkstring` is a length-tracked, growable string buffer for C. It avoids the
classic C string pitfalls — no repeated `strlen()` scans, no surprise
reallocations on every append — while staying close to the metal: a `cstr`
field gives you a plain, null-terminated `char *` whenever you need to hand
the contents to a libc or POSIX function.

_This project was conceived and designed by a human. After a sizable portion
of its functionality had been coded, the aforementioned human started using
Claude Code to accelerate development._

## Design notes

- Every `fkstring` tracks its length (`len`) and allocated capacity (`alloc`)
  explicitly, so most operations are O(1) or O(n) in the data size, not in
  repeated scans for a terminating NUL.
- Buffers grow and shrink geometrically: appends over-allocate by a bump
  factor, and truncation only `realloc()`s once a deflate factor is crossed.
  This amortizes the cost of repeated appends/truncations. See
  `fkstring_internal.h` for the tunable globals (`_bumpfactor`,
  `_deflatefactor`, `_minalloc`, `_sprintftry`, `_slurptry`,
  `_catbufsize`).
- Buffers are kept null-terminated after `len` bytes as a convenience for
  interop with C APIs, but `len` is authoritative — `fkstrnewb()` lets you
  store binary data containing embedded NUL bytes, and the length-aware
  functions (`fkstrcat`, `fksubstr`, `fkremove`, `fkstrwrite`, ...) all
  respect `len` rather than scanning for a NUL.
- Lengths, sizes, and byte counts use `size_t`; functions that can return a
  byte count or a negative error use `ssize_t` (mirroring `read()`/`write()`
  conventions).
- On allocation failure, the library calls `fkpanic()`, which prints a
  message to stderr and calls `exit()`. fkstring is not designed to recover
  from out-of-memory conditions. Length arithmetic that would overflow
  `size_t` is treated the same way.

## Building and installing

fkstring uses the GNU Autotools. From a git checkout, generate `configure`
first (this needs autoconf, automake and libtool); release tarballs made
with `make dist` already include it:

```sh
autoreconf -fi
./configure
make
sudo make install
```

This builds and installs `libfkstring.a` (static) and `libfkstring.so`
(shared, via libtool) under `$prefix/lib`, the headers (`fkstring.h`,
`fkstring_internal.h`) under `$prefix/include`, and the manual pages under
`$prefix/share/man/man3`. `prefix` defaults to
`/usr/local`; pass `--prefix=DIR` to `configure` to install elsewhere, and
the usual `--disable-shared`/`--disable-static` to build only one flavor.
Out-of-tree builds (running `configure` from another directory) work too.

`configure` checks for `memmem()` and defines `FKSTR_HAVE_MEMMEM`
accordingly. To force the portable `memchr()`+`memcmp()` fallback, run
`./configure ac_cv_func_memmem=no`.

To use the library in your own project:

```c
#include <fkstring.h>
```

```sh
gcc myprog.c -lfkstring -o myprog
```

## Versioning

fkstring follows [Semantic Versioning](https://semver.org/). The current
release is 0.9.0: usable and fully tested, but while the major version is 0
the API may still change incompatibly between minor releases. `NEWS` lists
the changes in each release.

`fkstring.h` defines the version your program is compiled against as
`FKSTRING_VERSION` (a string, e.g. `"0.9.0"`), its components
`FKSTRING_VERSION_MAJOR`, `_MINOR` and `_PATCH`, and
`FKSTRING_VERSION_NUMBER` (`major * 10000 + minor * 100 + patch`) for
preprocessor checks. At run time, `const char *fkstrversion(void)` returns
the version of the library actually loaded:

```c
#if FKSTRING_VERSION_NUMBER < 900
#error "fkstring 0.9.0 or later is required"
#endif
```

The shared library's soname (`libfkstring.so.0`) is versioned separately,
with libtool's `-version-info`, and changes only when the binary interface
breaks.

## The fkstring type

```c
typedef struct _fkstring
{
	size_t	len;	/* number of bytes of content, excluding the trailing NUL */
	size_t	alloc;	/* allocated capacity of cstr, in bytes */
	char	*cstr;	/* null-terminated buffer; NULL when len == 0 */
} fkstring;
```

Accessor macros:

| Macro              | Description                                  |
|--------------------|-----------------------------------------------|
| `fkstrlen(fks)`     | Length in bytes (`size_t`).                   |
| `fkstrsize(fks)`    | Allocated capacity in bytes (`size_t`).       |
| `fkcstr(fks)`       | The underlying `char *` buffer.               |

## Function reference

Every function also has a manual page in section 3: start with
`man fkstring` for an overview, or `man fkstrcat` etc. for a specific
function. Related functions share a page. To read the pages without
installing, run `man -l man/fkstrcat.3`.

### Creation and destruction

#### `fkstring *fkstrnew(const char *s);`
Creates a new `fkstring` from a null-terminated C string `s`. Passing `NULL`
or an empty string creates an empty `fkstring` (`len == 0`, `cstr == NULL`).

#### `fkstring *fkstrnewb(const void *buf, size_t len);`
Creates a new `fkstring` from `len` bytes at `buf`. Unlike `fkstrnew()`, the
input does not need to be null-terminated and may contain embedded NUL
bytes — use this for binary data. Passing `NULL` or `len == 0` creates an
empty `fkstring`.

#### `fkstring *fkalloc(size_t size);`
#### `fkstring *fkcalloc(size_t nmemb, size_t size);`
Create a new `fkstring` of length `size` (`fkalloc()`) or `nmemb * size`
(`fkcalloc()`), modeled on `malloc()` and `calloc()`. `fkalloc()` leaves the
`len` bytes uninitialized for the caller to fill in (e.g. with `memcpy()` or
`read()`); `fkcalloc()` zeroes them, and they count as embedded NULs within
`len`. Either way `cstr[len]` is a NUL terminator. A length of 0 creates an
empty `fkstring` (`len == 0`, `cstr == NULL`). Unlike `calloc()`, an
`nmemb * size` overflow doesn't return `NULL`; like out-of-memory, it's fatal
(see [Design notes](#design-notes)).

#### `fkstring *fkstrdup(const fkstring *fks);`
Creates a new `fkstring` as a copy of `fks`.

#### `void fkstrdestroy(fkstring *fks);`
Deallocates `fks` and its buffer. As with `free()`, the pointer is invalid
after this call. Safe to call with `NULL`.

#### `void fkarraydestroy(fkstring **fka);`
Deallocates every `fkstring` in the NULL-terminated array `fka` (as returned
by `fksplit()`), then the array itself. Safe to call with `NULL`.

### Modifying in place

#### `fkstring *fkstrcat(fkstring *dst, const fkstring *src);`
Appends `src` to `dst` in place. Returns `dst`, or `NULL` if either argument
is `NULL`.

#### `fkstring *fkstrcatc(fkstring *dst, const char *src);`
Appends a null-terminated C string `src` to `dst` in place. A `NULL` or empty
`src` is a no-op. Returns `dst`, or `NULL` if `dst` is `NULL`.

#### `fkstring *fkstrcatone(fkstring *dst, char c);`
Appends a single character `c` to `dst` in place. Returns `dst`, or `NULL` if
`dst` is `NULL`.

The `src` of any of the `fkstrcat` functions may be `dst` itself, or point
into `dst`'s buffer: `fkstrcat(s, s)` doubles `s`.

#### `fkstring *fkinsert(fkstring *fks, size_t pos, const fkstring *src);`
#### `fkstring *fkinsertc(fkstring *fks, size_t pos, const char *cstr);`
Insert `src` (or the null-terminated C string `cstr`) into `fks` at byte
offset `pos`, in place, shifting the bytes from `pos` onward up. `pos ==
fkstrlen(fks)` appends. Returns `fks` for chaining, or `NULL`, leaving `fks`
unchanged, if any argument is `NULL` or `pos > fkstrlen(fks)`. Inserting an
empty string is a no-op. As with `fkstrcat()`, `src` may be `fks` itself or
point into its buffer. `fkinsert()` is the counterpart of `fkremove()`:

```c
fkstring *s = fkstrnew("Demon");
fkinsertc(s, 3, "oti"); /* "Demon" -> "Demotion" */
```

#### `fkstring *fkreplace(fkstring *fks, const fkstring *old, const fkstring *new, size_t max_count);`
#### `fkstring *fkreplacec(fkstring *fks, const char *old, const char *new, size_t max_count);`
Replace occurrences of `old` in `fks` with `new`, in place, scanning left to
right. Matches don't overlap (`"aaa"` holds one `"aa"`), and replacement text
is never rescanned. At most `max_count` matches are replaced, or all of them
if `max_count` is 0. An empty `old` matches nothing, so the call is a no-op.
Matching and replacement honor embedded NULs. The matches are counted first,
so `fks` is reallocated at most once however many there are, and not at all
when nothing matches. `old` and `new` may alias `fks`. Returns `fks` for
chaining, or `NULL` if any argument is `NULL`.

```c
fkstring *s = fkstrnew("a-b-c");
fkreplacec(s, "-", ", ", 0); /* "a, b, c" */
fkreplacec(s, ", ", "", 1);  /* "ab, c" */
```

#### `fkstring *fkstrcatf(fkstring *dst, const char *fmt, ...);`
#### `fkstring *fkstrcatvf(fkstring *dst, const char *fmt, va_list ap);`
Append `printf()`-formatted output to `dst` in place, writing directly into
`dst`'s buffer past its current contents. If the output fits in the existing
spare capacity, nothing is reallocated. Otherwise `dst` grows once, by the
usual bump factor, and the output is formatted again. Returns `dst` for
chaining, or `NULL` if `dst` or `fmt` is `NULL`. `%c` with a zero argument
appends an embedded NUL, and `len` counts it. If `vsnprintf()` itself fails
(output past `INT_MAX` bytes, or a `%ls`/`%lc` argument the current locale
can't convert), they return `NULL` with `errno` set (`EOVERFLOW`/`EILSEQ`)
and `dst` unchanged.

`fkstrcatvf()` takes a `va_list` so you can write your own variadic
wrappers. As with `vprintf()`, `ap` is indeterminate afterwards: call
`va_end()` on it and don't reuse it.

```c
static void logmsg(fkstring *log, const char *fmt, ...)
{
	va_list ap;

	fkstrcatc(log, "[log] ");
	va_start(ap, fmt);
	fkstrcatvf(log, fmt, ap);
	va_end(ap);
	fkstrcatone(log, '\n');
}
```

#### `fkstring *fkstrtrunc(fkstring *fks, size_t newlen);`
Truncates `fks` to `newlen` bytes in place. Returns `fks`, or `NULL` if
`fks` is `NULL`. No-op if `newlen >= fkstrlen(fks)`.

#### `fkstring *fkslack(fkstring *fks, size_t n);`
Ensures `fks` has room for at least `n` more bytes past its current length,
growing its buffer to exactly `fkstrlen(fks) + n + 1` bytes if needed, so
that the next `n` bytes of appends or inserts don't reallocate. Returns
`fks`, or `NULL` if `fks` is `NULL`. It's a no-op on an empty `fks`, which
never has a buffer; to preallocate one, use `fkalloc()` or `fkcalloc()`.
Slack isn't sticky: a later `fkstrtrunc()`, `fkremove()`, or trim can
shrink the buffer again once it crosses the deflate threshold (see
[Design notes](#design-notes)).

```c
fkstring *s = fkstrnew("Log:");
fkslack(s, 4096);        /* one realloc() now... */
for (i = 0; i < n; i++)
	fkstrcatf(s, " %d", v[i]); /* ...none here while it fits */
```

#### `fkstring *fkfit(fkstring *fks);`
Shrinks the buffer of `fks` to exactly `fkstrlen(fks) + 1` bytes, releasing
any slack. Unlike the shrinking that `fkstrtrunc()` does, this ignores the
deflate threshold and the minimum allocation size. Use it once a string is
done growing and will be kept around. Returns `fks`, or `NULL` if `fks` is
`NULL`. No-op on an empty `fks`.

#### `size_t fkremove(fkstring *fstr, size_t start, size_t len);`
Removes up to `len` bytes starting at byte offset `start` from `fstr`, in
place, shifting the remaining bytes down. Returns the number of bytes
actually removed (0 if `fstr` is `NULL`, `start` is out of range, or `len`
is 0).

```c
fkstring *s = fkstrnew("Demotion");
fkremove(s, 3, 3); /* "Demotion" -> "Demon" */
```

#### `size_t fkltrim(fkstring *fks);`
#### `size_t fkrtrim(fkstring *fks);`
#### `size_t fktrim(fkstring *fks);`
Remove whitespace (the regex `\s` class: space, `\t`, `\n`, `\r`, `\f`, `\v`)
from the left, right, or both ends of `fks` in place. Returns the new
length (0 if `fks` is `NULL`).

### Deriving new strings

#### `fkstring *fksubstr(const fkstring *fstr, size_t start, size_t len);`
Returns a new `fkstring` containing up to `len` bytes of `fstr` starting at
byte offset `start`. Returns an empty `fkstring` if `start` is out of range
or `len` is 0, and `NULL` if `fstr` is `NULL`.

#### `fkstring **fksplit(const fkstring *src, char delim);`
Splits `src` on every occurrence of the byte `delim`, returning a
NULL-terminated array of newly allocated `fkstring`s (free with
`fkarraydestroy()`). An empty `src` yields a single-element array holding
one empty `fkstring`. Returns `NULL` if `src` is `NULL`.

```c
fkstring *s = fkstrnew("a,b,c");
fkstring **parts = fksplit(s, ','); /* {"a", "b", "c", NULL} */
fkarraydestroy(parts);
```

#### `fkstring *fkjoin(fkstring **arr, const char *sep);`
Inverse of `fksplit()`: concatenates the elements of the NULL-terminated
array `arr` into a newly allocated `fkstring`, with the C string `sep` placed
between consecutive elements (not before the first or after the last). Empty
elements still get their separators, so `fkjoin(fksplit(s, ','), ",")`
reproduces `s`. The total length is computed first, so the result is
allocated only once. Embedded NULs in the elements are preserved. A `NULL`
`sep` is treated as `""`. An empty array (just the `NULL` terminator) yields
an empty `fkstring`. Returns `NULL` if `arr` is `NULL`. `arr` and its
elements are left untouched.

```c
fkstring **parts = fksplit(s, ',');
fkstring *csv = fkjoin(parts, ", "); /* "a, b, c" */
fkarraydestroy(parts);
fkstrdestroy(csv);
```

#### `fkstring *fksprintf(const char *fmt, ...);`
#### `fkstring *fkvsprintf(const char *fmt, va_list ap);`
Create a new `fkstring` formatted using `printf()` semantics. They are
equivalent to `fkstrcatf()`/`fkstrcatvf()` on a new empty string, and return
`NULL` if `fmt` is `NULL`, or with `errno` set if `vsnprintf()` fails.

### Comparison

#### `int fkstrcmp(const fkstring *a, const fkstring *b);`
#### `int fkstrcasecmp(const fkstring *a, const fkstring *b);`
Compare `a` and `b` byte by byte (as `unsigned char`, like `memcmp()`) over
the shorter of the two lengths; if that prefix is equal, the shorter string
sorts first. Returns -1, 0, or 1. Unlike `strcmp(fkcstr(a), fkcstr(b))`,
these compare past embedded NUL bytes and are safe on empty strings (whose
`cstr` is `NULL`). A `NULL` argument sorts before any `fkstring`, including
an empty one; two `NULL`s compare equal. `fkstrcasecmp()` folds ASCII
`A`–`Z` to lowercase only. It is locale-independent and does not fold
non-ASCII bytes.

#### `int fkstreq(const fkstring *a, const fkstring *b);`
Returns 1 if `a` and `b` have the same length and contents, 0 otherwise.
Returns 0 immediately when the lengths differ, without looking at the
contents. `fkstreq(NULL, NULL)` is 1; `NULL` never equals a non-`NULL`
`fkstring`.

```c
fkstring *a = fkstrnew("Hello"), *b = fkstrnew("hello");
fkstreq(a, b);      /* 0 */
fkstrcmp(a, b);     /* -1: 'H' (0x48) < 'h' (0x68) */
fkstrcasecmp(a, b); /* 0 */
```

### Searching

The search functions return a byte offset into the haystack, or the
sentinel `FKSTR_NPOS` (`(size_t)-1`) when there is no match or an argument
is `NULL`. Like the comparison functions, they honor `len`: they match
across embedded NUL bytes, never match the trailing NUL at `cstr[len]`,
and are safe on empty strings.

#### `size_t fkstrfind(const fkstring *hay, const fkstring *needle, size_t start);`
#### `size_t fkstrfindc(const fkstring *hay, const char *needle, size_t start);`
Return the offset of the first occurrence of `needle` in `hay` at or after
byte offset `start`. `fkstrfindc()` takes a null-terminated C string as the
needle; the haystack is still searched in full, past any embedded NULs.
An empty needle matches at `start` itself, provided `start <= fkstrlen(hay)`
(the same rule as C++'s `std::string::find`). Uses `memmem()` where the
platform provides it (glibc, the BSDs, macOS, POSIX.1-2024), with a portable
fallback otherwise. Build with `-DFKSTR_HAVE_MEMMEM=0` or `=1` to override
the detection.

```c
fkstring *s = fkstrnew("aXbXXc"), *x = fkstrnew("X");
size_t pos;

for (pos = fkstrfind(s, x, 0); pos != FKSTR_NPOS; pos = fkstrfind(s, x, pos + 1))
	printf("%zu\n", pos); /* 1, 3, 4 */
```

#### `size_t fkstrchr(const fkstring *fks, char c, size_t start);`
Returns the offset of the first byte equal to `c` at or after byte offset
`start`, or `FKSTR_NPOS` if there is none or `start >= fkstrlen(fks)`.

#### `size_t fkstrrchr(const fkstring *fks, char c);`
Returns the offset of the last byte equal to `c`.

#### `int fkstartswith(const fkstring *fks, const fkstring *prefix);`
#### `int fkstartswithc(const fkstring *fks, const char *prefix);`
#### `int fkendswith(const fkstring *fks, const fkstring *suffix);`
#### `int fkendswithc(const fkstring *fks, const char *suffix);`
Return 1 if `fks` begins (or ends) with `prefix` (or `suffix`), 0
otherwise. The empty string is a prefix and suffix of every `fkstring`.
Return 0 if either argument is `NULL`. The `c` variants take a
null-terminated C string as the prefix/suffix; `fks` itself is still
compared by `len`, so a suffix match lines up with the true end of `fks`,
past any embedded NULs.

```c
fkstring *f = fkstrnew("file.tar.gz"), *gz = fkstrnew(".gz");
fkendswith(f, gz);          /* 1 */
fkendswithc(f, ".tar.gz");  /* 1 */
fkstartswithc(f, "file.");  /* 1 */
```

### I/O

#### `ssize_t fkstrwrite(int fd, const fkstring *fks);`
Writes the contents of `fks` to file descriptor `fd`. Returns the result of
the underlying `write()` call.

#### `fkstring *fkstrread(int fd, size_t count);`
Creates a new `fkstring` by reading up to `count` bytes from file
descriptor `fd`. Returns `NULL` on a read error (negative return from
`read()`); returns an empty `fkstring` at end of file. `count == SIZE_MAX` is
a fatal length overflow, like out-of-memory.

#### `fkstring *fkreadline(FILE *fp);`
Reads one line of any length from `fp` and returns it as a new `fkstring`.
The trailing `'\n'` is kept, so the only line without one is a last line
that has no newline at the end. Embedded NUL bytes are preserved. Returns
`NULL` if `fp` is `NULL`, or if end of file or a read error comes before any
byte is read. This makes the usual loop
`while ((line = fkreadline(fp)) != NULL)` work. It differs from
`fkstrread()`, which returns an empty `fkstring` at EOF. If an error happens
partway through a line, the bytes read so far are returned; use
`ferror(fp)` to tell that apart from a normal EOF. The stream is locked once
per line with `flockfile()`, and each byte is read with `getc_unlocked()`.

#### `fkstring *fkslurp(int fd);`
Reads everything from file descriptor `fd`, from its current offset to end
of file, into a new `fkstring`. Embedded NUL bytes are preserved. For a
regular file, the size from `fstat()` (minus the current offset) sets the
initial allocation, so the whole file is read into one buffer, with no
reallocation. The size is only a hint, though: reading always continues
until `read()` reports EOF. So a file that grows while it's being read, or a
`/proc` file that reports a size of 0, is still read in full. Pipes,
sockets, and terminals have no size hint. For those, reading starts with a
`_slurptry`-byte (4 KiB) buffer that grows by the usual bump factor. Reads
interrupted by a signal (`EINTR`) are retried. When a read fills much less
of the buffer than was allocated, the excess is released, using the same
threshold as `fkstrtrunc()`. Returns an empty `fkstring` if there is nothing
to read. Returns `NULL` on a read error, with `errno` set by `read()`, and
discards any bytes already read. This includes `EAGAIN` on a non-blocking
`fd`.

#### `fkstring *fkslurpfile(const char *path);`
Opens `path` read-only, reads it in full with `fkslurp()`, and closes it.
Returns `NULL` if `path` is `NULL`, if `open()` fails, or if the read fails.
`errno` is preserved from the failing call.

**Warning:** both functions read until end of file, and some files never
end. Don't use them on `/dev/zero`, `/dev/urandom`, a pipe from `yes`, or
anything else that never reports EOF. The buffer keeps growing until
`malloc()` fails and `fkpanic()` exits, or the machine starts swapping or
the kernel's OOM killer steps in first. You'd end up with an `fksystem`
instead of an `fkstring`. For those, use `fkstrread()` with an explicit
byte count.

```c
fkstring *conf = fkslurpfile("/etc/hostname");
if (conf)
	fkstrwrite(1, conf);
fkstrdestroy(conf);
```

#### `ssize_t fkcatfd(const char *path, int fd);`
#### `ssize_t fkcatf(const char *path, FILE *f);`
#### `fkcat(path)`
Copy the contents of the file at `path` to file descriptor `fd`
(`fkcatfd()`) or to stream `f` (`fkcatf()`). `fkcat(path)` is a macro for
`fkcatfd(path, STDOUT_FILENO)`. No `fkstring` is built: the file is streamed
through a single `_catbufsize`-byte (64 KiB) buffer. Memory use therefore
doesn't depend on the file's size, and the `fkslurp()` warning about files
that never end doesn't apply (though `fkcat("/dev/zero")` still never
returns). Embedded NUL bytes are copied as-is. Reads interrupted by a signal
(`EINTR`) are retried, and so are short writes to `fd`.

Returns the number of bytes copied, capped at `SSIZE_MAX`. Returns -1 with
`errno` set if `path` is `NULL` or `f` is `NULL` (`EINVAL`), if `open()`
fails, if a read fails (e.g. `EISDIR` for a directory), or if a write fails.
Bytes already copied before a failure stay written.

`fkcatf()` writes with `fwrite()`, so its output follows anything already
buffered in `f`. It may still be sitting in `f`'s buffer when the function
returns; call `fflush(f)` if that matters. `fkcatfd()` bypasses stdio
entirely. So before mixing it with `printf()` on the same descriptor (e.g.
`fkcat()` after `printf()`), `fflush(stdout)`, or the output can come out
of order.

```c
if (fkcat("/etc/hostname") < 0)
	perror("/etc/hostname");
```

## Tests

The `tests/` directory holds an automated test suite covering every public
function. Run it with:

```sh
make check
```

This also runs `tests/manpages.sh`, which checks that every public function
has a manual page and, if groff is installed, that the pages have no
formatting warnings. The suite's per-test output goes to `tests/alltests.log`; run
`./tests/alltests` directly to watch it live.

To also check for memory leaks and errors under Valgrind (which must be
installed when `configure` runs), run:

```sh
make memcheck
```
