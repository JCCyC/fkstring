#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
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
