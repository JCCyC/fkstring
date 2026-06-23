#include <unistd.h>
#include <stdlib.h>
#include <fkstring.h>
#include <stdio.h>
#include <fkstring_internal.h>

int _bumpfactor = FKSTR_DEFAULT_BUMPFACTOR;
int _deflatefactor = FKSTR_DEFAULT_DEFLATEFACTOR;
int _minalloc = FKSTR_DEFAULT_MIN_ALLOC;
int _sprintftry = FKSTR_DEFAULT_SPRINTF_TRY;

void fkpanic(int cause)
{
	if (cause)
		write(2, errmsgs[cause], strlen(errmsgs[cause]));
	exit(253);
}

int allocforlen(int len)
{
	int proposed = (len * _bumpfactor / 100) + 1;

	if (proposed < _minalloc)
		proposed = _minalloc;
	return proposed;
}

void fkstrdestroy(fkstring *fks)
{
	if (fks)
	{
		if (fks->cstr)
			free(fks->cstr);
		free(fks);
	}
}

fkstring *fkstrnew(const char *s)
{
	int		newlen, newalloc;
	fkstring	*newfkstr;

	newfkstr = malloc(sizeof(fkstring));
	if (!newfkstr)
		fkpanic(FKSTRERR_MEMALLOC);

	if (s && s[0])
	{
		newlen = strlen(s);
		newalloc = allocforlen(newlen);

		newfkstr->cstr = malloc(newalloc);
		if (!newfkstr->cstr)
			fkpanic(FKSTRERR_MEMALLOC);

		memcpy(newfkstr->cstr, s, newlen + 1);
		newfkstr->len = newlen;
		newfkstr->alloc = newalloc;
	}
	else
	{
		newfkstr->len = 0;
		newfkstr->alloc = 0;
		newfkstr->cstr = NULL;
	}

	return newfkstr;
}

fkstring *fkstrtrunc(fkstring *fks, int newlen)
{
	if ((!fks) || (newlen < 0))
		return NULL;

	if (fks->len == 0)
		return fks;

	if (newlen == 0)
	{
		fks->len = fks->alloc = 0;
		free(fks->cstr);
		fks->cstr = NULL;
	}
	else if (newlen < fks->len)
	{
		fks->len = newlen;
		fks->cstr[newlen] = '\0';
		if ((fks->alloc > (newlen * _deflatefactor / 100)) && (fks->alloc > _minalloc))
		{
			char *newbuf;
			fks->alloc = newlen + 1;
			if (fks->alloc < _minalloc)
				fks->alloc = _minalloc;
			newbuf = realloc(fks->cstr, fks->alloc);
			if (!newbuf)
				fkpanic(FKSTRERR_MEMALLOC);
			fks->cstr = newbuf;
		}
	}
	return fks;
}

fkstring *fkstrdup(fkstring *fks)
{
	int		newlen, newalloc;
	fkstring	*newfkstr;

	if (!fks)
		return NULL;

	newlen = fks->len;
	if (newlen <= 0)
		return fkstrnew(NULL);

	newfkstr = malloc(sizeof(fkstring));
	if (!newfkstr)
		fkpanic(FKSTRERR_MEMALLOC);

	newalloc = allocforlen(newlen);

	newfkstr->cstr = malloc(newalloc);
	if (!newfkstr->cstr)
		fkpanic(FKSTRERR_MEMALLOC);

	memcpy(newfkstr->cstr, fks->cstr, newlen + 1);
	newfkstr->len = newlen;
	newfkstr->alloc = newalloc;

	return newfkstr;
}

/* Assumes src has nonzero length, but dst may have zero length. */

static fkstring *fkstrcat_internal(fkstring *dst, char *src, int srclen)
{
	int newalloc;
	char *newbuf;
	
	if ((dst->alloc - dst->len) <= (srclen))
	{
		newalloc = allocforlen(dst->len + srclen);
		if (dst->len)
			newbuf = realloc(dst->cstr, newalloc);
		else
			newbuf = malloc(newalloc);

		if (!newbuf)
			fkpanic(FKSTRERR_MEMALLOC);

		dst->cstr = newbuf;
		dst->alloc = newalloc;
	}
	memcpy(&dst->cstr[dst->len], src, srclen);
	dst->len += srclen;
	dst->cstr[dst->len] = '\0';

	return dst;
}

fkstring *fkstrcat(fkstring *dst, fkstring *src)
{
	if (src->len)
	{
		return fkstrcat_internal(dst, src->cstr, src->len);
	}
	else
	{
		return dst;
	}
}

fkstring *fkstrcatc(fkstring *dst, char *src)
{
	if (src && src[0])
		return fkstrcat_internal(dst, src, strlen(src));
	else
		return dst;
}

fkstring *fkstrcatone(fkstring *dst, char c)
{
	return fkstrcat_internal(dst, &c, 1);
}

int fkremove(fkstring *fstr, int start, int len)
{

	if ((start < 0) || (len < 0) || (!fstr))
		return 0;

	if ((start >= fstr->len) || (len == 0))
		return 0;

	if (len > (fstr->len - start))
		len = fstr->len - start;

	if (len < (fstr->len - start))
		memcpy(&fstr->cstr[start], &fstr->cstr[start + len], fstr->len - start - len);

	if (!fkstrtrunc(fstr, fstr->len - len))
		return 0;

	return len;
}

fkstring *fksubstr(fkstring *fstr, int start, int len)
{
	int		newalloc;
	fkstring	*newfkstr;

	if ((start < 0) || (len < 0) || (!fstr))
		return NULL;

	if ((start >= fstr->len) || (len == 0))
		return fkstrnew(NULL);

	if (len > (fstr->len - start))
		len = fstr->len - start;

	newfkstr = malloc(sizeof(fkstring));
	if (!newfkstr)
		fkpanic(FKSTRERR_MEMALLOC);

	newalloc = allocforlen(len);

	newfkstr->cstr = malloc(newalloc);
	if (!newfkstr->cstr)
		fkpanic(FKSTRERR_MEMALLOC);

	memcpy(newfkstr->cstr, &fstr->cstr[start], len);
	newfkstr->cstr[len] = '\0';
	newfkstr->len = len;
	newfkstr->alloc = newalloc;

	return newfkstr;
}
