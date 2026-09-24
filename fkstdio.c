#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fkstring.h>
#include <fkstring_internal.h>

/*
 * Two-pass vsnprintf(), formatting directly at dst->cstr + dst->len. The
 * first pass uses whatever slack dst already has (an empty dst gets a
 * _sprintftry-byte guess); if vsnprintf() reports it needed more, dst grows
 * via allocforlen() and the second pass formats again, consuming ap itself
 * (the first pass used a va_copy()). Like vprintf(), ap is indeterminate
 * afterwards.
 */
fkstring *fkstrcatvf(fkstring *dst, const char *fmt, va_list ap)
{
	va_list	ap2;
	size_t	avail, need, newalloc;
	char	*newbuf;
	int	n;

	if (!dst || !fmt)
		return NULL;

	if (dst->len == 0)
	{
		if ((dst->cstr = malloc(_sprintftry)) == NULL)
			fkpanic(FKSTRERR_MEMALLOC);
		dst->alloc = _sprintftry;
	}

	avail = dst->alloc - dst->len;
	va_copy(ap2, ap);
	n = vsnprintf(dst->cstr + dst->len, avail, fmt, ap2);
	va_end(ap2);

	if (n < 0 || (size_t)n >= avail)
	{
		if (n > -1)
			need = fkaddlen(dst->len, (size_t)n);	/* glibc >= 2.1 - precisely what is needed */
		else
		{
			/* glibc 2.0 - ten times the old size.
			 * TODO: on glibc < 2.1, output longer than that still fails. */
			if (avail > SIZE_MAX / 10)
				fkpanic(FKSTRERR_OVERFLOW);
			need = fkaddlen(dst->len, avail * 10);
		}

		newalloc = allocforlen(need);
		if ((newbuf = realloc(dst->cstr, newalloc)) == NULL)
			fkpanic(FKSTRERR_MEMALLOC);
		dst->cstr = newbuf;
		dst->alloc = newalloc;

		avail = dst->alloc - dst->len;
		n = vsnprintf(dst->cstr + dst->len, avail, fmt, ap);
		if (n < 0 || (size_t)n >= avail)
			fkpanic(FKSTRERR_VSNPRINTF);
	}

	dst->len += (size_t)n;
	if (dst->len == 0)
	{
		/* Empty output onto an empty dst: restore the len == 0 invariant */
		free(dst->cstr);
		dst->cstr = NULL;
		dst->alloc = 0;
	}
	return dst;
}

fkstring *fkstrcatf(fkstring *dst, const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	dst = fkstrcatvf(dst, fmt, ap);
	va_end(ap);
	return dst;
}

fkstring *fkvsprintf(const char *fmt, va_list ap)
{
	fkstring	*newfkstr;

	if (!fmt)
		return NULL;

	newfkstr = fkstrnew(NULL);
	return fkstrcatvf(newfkstr, fmt, ap);
}

fkstring *fksprintf(const char *fmt, ...)
{
	fkstring	*newfkstr;
	va_list		ap;

	va_start(ap, fmt);
	newfkstr = fkvsprintf(fmt, ap);
	va_end(ap);
	return newfkstr;
}

ssize_t fkstrwrite(int fd, const fkstring *fks)
{
	if (!fks || fks->len == 0)
		return write(fd, "", 0);
	else
		return write(fd, fks->cstr, fks->len);
}

fkstring *fkstrread(int fd, size_t count)
{
	fkstring	*newfkstr;
	size_t		myalloc;
	ssize_t		bytesread;

	newfkstr = malloc(sizeof(fkstring));
	if (!newfkstr)
		fkpanic(FKSTRERR_MEMALLOC);
	myalloc = count + 1;
	newfkstr->alloc = myalloc;
	newfkstr->cstr = malloc(myalloc);
	if (!newfkstr->cstr)
	{
		free(newfkstr);
		fkpanic(FKSTRERR_MEMALLOC);
	}
	bytesread = read(fd, newfkstr->cstr, count);
	if (bytesread < 0)
	{
		free(newfkstr->cstr);
		free(newfkstr);
		newfkstr = NULL;
	}
	else if (bytesread == 0)
	{
		free(newfkstr->cstr);
		newfkstr->cstr = NULL;
		newfkstr->len = 0;
		newfkstr->alloc = 0;
	}
	else
	{
		newfkstr->cstr[bytesread] = '\0';
		newfkstr->len = (size_t)bytesread;
	}
	return newfkstr;
}

/*
 * Reads one line of arbitrary length from fp, including its trailing '\n'
 * (absent only on a final line not ending in one). Embedded NULs are kept.
 * Returns NULL for a NULL fp, or when EOF or an error comes before any byte
 * is read; a partial line read before an error is returned as-is, so check
 * ferror(fp) to tell the two apart. The stream is locked once for the whole
 * line so each byte can use getc_unlocked().
 */
fkstring *fkreadline(FILE *fp)
{
	fkstring	*line;
	size_t		newalloc;
	char		*newbuf;
	int		c;

	if (!fp)
		return NULL;

	line = fkstrnew(NULL);
	flockfile(fp);
	while ((c = getc_unlocked(fp)) != EOF)
	{
		if (line->len + 1 >= line->alloc)
		{
			newalloc = allocforlen(fkaddlen(line->len, 1));
			if ((newbuf = realloc(line->cstr, newalloc)) == NULL)
				fkpanic(FKSTRERR_MEMALLOC);
			line->cstr = newbuf;
			line->alloc = newalloc;
		}
		line->cstr[line->len++] = (char)c;
		if (c == '\n')
			break;
	}
	funlockfile(fp);

	if (line->len == 0)
	{
		fkstrdestroy(line);
		return NULL;
	}
	line->cstr[line->len] = '\0';
	return line;
}

/*
 * Reads fd from its current offset to EOF. For a regular file, fstat()'s
 * size minus the current offset only sizes the first allocation: the file
 * can change under us, and /proc files report 0. So this always reads until
 * read() returns 0, growing via allocforlen() past the hint. The hinted
 * buffer has one spare byte so the EOF-confirming read() needs no growth.
 * Excess slack (e.g. a short pipe read into a _slurptry buffer) is released
 * past the same _deflatefactor threshold fkstrtrunc() uses.
 */
fkstring *fkslurp(int fd)
{
	fkstring	*fks;
	struct stat	st;
	off_t		pos;
	size_t		newalloc;
	char		*newbuf;
	ssize_t		n;
	int		saved_errno;

	newalloc = _slurptry;
	if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode) &&
	    (pos = lseek(fd, 0, SEEK_CUR)) >= 0 && st.st_size > pos)
	{
		if ((uintmax_t)(st.st_size - pos) > SIZE_MAX - 2)
			fkpanic(FKSTRERR_OVERFLOW);
		newalloc = (size_t)(st.st_size - pos) + 2;
	}

	fks = fkstrnew(NULL);
	if ((fks->cstr = malloc(newalloc)) == NULL)
		fkpanic(FKSTRERR_MEMALLOC);
	fks->alloc = newalloc;

	for (;;)
	{
		if (fks->len + 1 >= fks->alloc)
		{
			newalloc = allocforlen(fkaddlen(fks->len, 1));
			if ((newbuf = realloc(fks->cstr, newalloc)) == NULL)
				fkpanic(FKSTRERR_MEMALLOC);
			fks->cstr = newbuf;
			fks->alloc = newalloc;
		}
		n = read(fd, fks->cstr + fks->len, fks->alloc - fks->len - 1);
		if (n > 0)
			fks->len += (size_t)n;
		else if (n == 0)
			break;
		else if (errno != EINTR)
		{
			saved_errno = errno;
			fkstrdestroy(fks);
			errno = saved_errno;
			return NULL;
		}
	}

	if (fks->len == 0)
	{
		free(fks->cstr);
		fks->cstr = NULL;
		fks->alloc = 0;
		return fks;
	}
	fks->cstr[fks->len] = '\0';
	if (fks->alloc > fks->len * _deflatefactor / 100 && fks->alloc > _minalloc)
		fkfit(fks);
	return fks;
}

fkstring *fkslurpfile(const char *path)
{
	fkstring	*fks;
	int		fd, saved_errno;

	if (!path)
		return NULL;
	if ((fd = open(path, O_RDONLY | O_CLOEXEC)) < 0)
		return NULL;
	fks = fkslurp(fd);
	saved_errno = errno;
	close(fd);
	errno = saved_errno;
	return fks;
}
