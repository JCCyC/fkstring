# fkstring

F\*\*\*ed Strings — a small C string library, inspired by
["Back to Basics"](https://www.joelonsoftware.com/2001/12/11/back-to-basics/).

`fkstring` is a length-tracked, growable string buffer for C. It avoids the
classic C string pitfalls — no repeated `strlen()` scans, no surprise
reallocations on every append — while staying close to the metal: a `cstr`
field gives you a plain, null-terminated `char *` whenever you need to hand
the contents to a libc or POSIX function.

## Design notes

- Every `fkstring` tracks its length (`len`) and allocated capacity (`alloc`)
  explicitly, so most operations are O(1) or O(n) in the data size, not in
  repeated scans for a terminating NUL.
- Buffers grow and shrink geometrically: appends over-allocate by a bump
  factor, and truncation only `realloc()`s once a deflate factor is crossed.
  This amortizes the cost of repeated appends/truncations. See
  `fkstring_internal.h` for the tunable globals (`_bumpfactor`,
  `_deflatefactor`, `_minalloc`, `_sprintftry`).
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

```sh
make
sudo make install
```

This builds `libfkstring.a` (static) and `libfkstring.so` (shared).
`make install` installs the libraries to
`$(PREFIX)/lib` and the headers (`fkstring.h`, `fkstring_internal.h`) to
`$(PREFIX)/include`, where `PREFIX` defaults to `/usr/local`. Edit `PREFIX`
in the `Makefile` to install elsewhere. The install step runs `install -o
root -g root`, so it needs root privileges.

To use the library in your own project:

```c
#include <fkstring.h>
```

```sh
gcc myprog.c -lfkstring -o myprog
```

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

### Creation and destruction

#### `fkstring *fkstrnew(const char *s);`
Creates a new `fkstring` from a null-terminated C string `s`. Passing `NULL`
or an empty string creates an empty `fkstring` (`len == 0`, `cstr == NULL`).

#### `fkstring *fkstrnewb(const void *buf, size_t len);`
Creates a new `fkstring` from `len` bytes at `buf`. Unlike `fkstrnew()`, the
input does not need to be null-terminated and may contain embedded NUL
bytes — use this for binary data. Passing `NULL` or `len == 0` creates an
empty `fkstring`.

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
Appends `src` to `dst` in place. Returns `dst`.

#### `fkstring *fkstrcatc(fkstring *dst, const char *src);`
Appends a null-terminated C string `src` to `dst` in place. Returns `dst`.

#### `fkstring *fkstrcatone(fkstring *dst, char c);`
Appends a single character `c` to `dst` in place. Returns `dst`.

#### `fkstring *fkstrcatf(fkstring *dst, const char *fmt, ...);`
#### `fkstring *fkstrcatvf(fkstring *dst, const char *fmt, va_list ap);`
Append `printf()`-formatted output to `dst` in place, writing directly into
`dst`'s buffer past its current contents. If the output fits in the existing
spare capacity, nothing is reallocated. Otherwise `dst` grows once, by the
usual bump factor, and the output is formatted again. Returns `dst` for
chaining, or `NULL` if `dst` or `fmt` is `NULL`. `%c` with a zero argument
appends an embedded NUL, and `len` counts it.

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
`NULL` if `fmt` is `NULL`.

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
`read()`); returns an empty `fkstring` at end of file.

## Tests / demo

The `tests/` directory holds an automated test suite covering every public
function. Run it with:

```sh
make check
```

To also check for memory leaks and errors under Valgrind (which must be
installed), run:

```sh
make memcheck
```
