#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <fkstring.h>
#include <fkstring_internal.h>

static char *make_message(size_t *len, size_t *alloc, const char *fmt, va_list ap)
{
	int n;
	size_t size;
	char *p;

	/* Try with size given to us */
	size = *len;

	if ((p = malloc(size)) == NULL)
		fkpanic(FKSTRERR_MEMALLOC);

	/* Try to print in the allocated space. */
	n = vsnprintf(p, size, fmt, ap);

	/* If that worked, return the string. */
	if (n > -1 && (size_t)n < size)
	{
		*len = (size_t)n;
		*alloc = size;
		return p;
	}

	/* Else try again with more space. */
	if (n > -1)
		size = (size_t)n + 1;	/* glibc 2.1 - precisely what is needed */
	else
		size *= 10;	/* glibc 2.0 - ten times the old size */

	/* Return NULL, instruct to try with new size */
	free(p);
	*len = size;
	return NULL;
}

fkstring *fksprintf(const char *fmt, ...)
{
	size_t		mylen, myalloc;
	char		*mycstr;
	fkstring	*newfkstr;
	va_list		ap;

	newfkstr = malloc(sizeof(fkstring));
	if (!newfkstr)
		fkpanic(FKSTRERR_MEMALLOC);

	mylen = _sprintftry;
	mycstr = NULL;
	va_start(ap, fmt);
	mycstr = make_message(&mylen, &myalloc, fmt, ap);
	va_end(ap);
	if (!mycstr)
	{
		/* TODO: In glibc < 2.1, fksprintf() will fail for strings longer than 10*FKSTR_DEFAULT_SPRINTF_TRY bytes */
		va_start(ap, fmt);
		mycstr = make_message(&mylen, &myalloc, fmt, ap);
		va_end(ap);
		if (!mycstr)
			fkpanic(FKSTRERR_VSNPRINTF);
	}

	if (mylen == 0)
	{
		free(mycstr);
		newfkstr->len = 0;
		newfkstr->alloc = 0;
		newfkstr->cstr = NULL;
	}
	else
	{
		newfkstr->alloc = myalloc;
		newfkstr->len = mylen;
		newfkstr->cstr = mycstr;
	}
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
