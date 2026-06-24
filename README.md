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
  from out-of-memory conditions.

## Building and installing

```sh
make
sudo make install
```

This builds `libfkstring.a` (static) and `libfkstring.so` (shared), plus the
`smoketest` demo program. `make install` installs the libraries to
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

### Modifying in place

#### `fkstring *fkstrcat(fkstring *dst, const fkstring *src);`
Appends `src` to `dst` in place. Returns `dst`.

#### `fkstring *fkstrcatc(fkstring *dst, const char *src);`
Appends a null-terminated C string `src` to `dst` in place. Returns `dst`.

#### `fkstring *fkstrcatone(fkstring *dst, char c);`
Appends a single character `c` to `dst` in place. Returns `dst`.

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

#### `fkstring *fksprintf(const char *fmt, ...);`
Creates a new `fkstring` formatted using `printf()` semantics.

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

`smoketest.c` is a separate manual demo program exercising the library
(concatenation, growth/shrink behavior, `fksprintf`, `fkremove`, stdin/fd
reading). Build it with `make` and run `./smoketest`.
